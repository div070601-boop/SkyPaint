package com.feather3d.app

import android.annotation.SuppressLint
import android.content.Context
import android.util.AttributeSet
import android.view.Choreographer
import android.view.MotionEvent
import android.view.SurfaceView
import kotlinx.coroutines.*
import kotlinx.coroutines.sync.*

/**
 * Custom SurfaceView hosting the Filament renderer and handling
 * touch/stylus input routing between camera and drawing.
 */
class Feather3DView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null
) : SurfaceView(context, attrs), Choreographer.FrameCallback {

    private val viewScope = CoroutineScope(SupervisorJob() + Dispatchers.Main)

    val filamentRenderer = FilamentRenderer(context)
    val cameraController = CameraController()
    val stylusInputHandler = StylusInputHandler(cameraController)

    // Callback to update UI with straight-line dimension
    var onDimensionUpdate: ((Float) -> Unit)? = null

    private var choreographer: Choreographer? = null
    private var isRunning = false
    private var currentDrawMode = NativeBridge.MODE_STROKE

    // Stats
    var onStatsUpdate: ((vertices: Int, triangles: Int) -> Unit)? = null

    fun init() {
        filamentRenderer.init(this)
        choreographer = Choreographer.getInstance()

        // Set up stylus listener
        stylusInputHandler.listener = object : StylusInputHandler.Listener {
            override fun onStrokeBegin() {
                NativeBridge.beginStroke()
            }

            // Ensure sequential dispatch to native C++ engine
            val engineMutex = Mutex()

            override fun onStrokePoint(
                worldX: Float, worldY: Float, worldZ: Float,
                pressure: Float, tilt: Float, timestamp: Float
            ) {
                viewScope.launch(Dispatchers.Default) {
                    engineMutex.withLock {
                        NativeBridge.addStrokePoint(worldX, worldY, worldZ, pressure, tilt, timestamp)

                        // Update live preview
                        val verts = NativeBridge.getCurrentStrokeMeshVertices()
                        val indices = NativeBridge.getCurrentStrokeMeshIndices()
                        
                        withContext(Dispatchers.Main) {
                            if (verts != null && indices != null) {
                                filamentRenderer.uploadCurrentStrokeMesh(verts, indices)
                            }
                            onDimensionUpdate?.invoke(NativeBridge.getCurrentStrokeLength())
                        }
                    }
                }
            }

            override fun onStrokeEnd() {
                viewScope.launch(Dispatchers.Default) {
                    engineMutex.withLock {
                        val strokeIndex = NativeBridge.endStroke()
                        
                        var verts: FloatArray? = null
                        var indices: IntArray? = null
                        
                        if (strokeIndex >= 0) {
                            verts = NativeBridge.getStrokeMeshVertices(strokeIndex)
                            indices = NativeBridge.getStrokeMeshIndices(strokeIndex)
                        }

                        withContext(Dispatchers.Main) {
                            if (strokeIndex >= 0 && verts != null && indices != null) {
                                filamentRenderer.uploadStrokeMesh(strokeIndex, verts, indices)
                            }
                            // Clear preview
                            filamentRenderer.uploadCurrentStrokeMesh(FloatArray(0), IntArray(0))
                            onDimensionUpdate?.invoke(-1f) // trigger hide
                            updateStats()
                        }
                    }
                }
            }

            override fun onSculptBegin() {
                viewScope.launch(Dispatchers.Default) {
                    engineMutex.withLock {
                        NativeBridge.beginSculpt()
                    }
                }
            }

            override fun onSculptPoint(
                worldX: Float, worldY: Float, worldZ: Float,
                pressure: Float
            ) {
                viewScope.launch(Dispatchers.Default) {
                    engineMutex.withLock {
                        val brushRadius = 0.05f * (0.5f + pressure)
                        NativeBridge.sculptAt(worldX, worldY, worldZ, brushRadius, pressure)

                        // Update voxel mesh visualization
                        val verts = NativeBridge.getVoxelMeshVertices()
                        val indices = NativeBridge.getVoxelMeshIndices()
                        
                        withContext(Dispatchers.Main) {
                            if (verts != null && indices != null) {
                                filamentRenderer.uploadVoxelMesh(verts, indices)
                            }
                            updateStats()
                        }
                    }
                }
            }

            override fun onSculptEnd() {
                viewScope.launch(Dispatchers.Default) {
                    engineMutex.withLock {
                        NativeBridge.endSculpt()
                    }
                }
            }
        }
    }


    fun addPrimitive(type: Int) {
        val identity = floatArrayOf(
            1f, 0f, 0f, 0f,
            0f, 1f, 0f, 0f,
            0f, 0f, 1f, 0f,
            0f, 0f, 0f, 1f
        )
        val id = NativeBridge.addPrimitive(type, identity, 0.5f, 0.5f, 0.5f, 1.0f)
        if (id >= 0) {
            val verts = NativeBridge.getPrimitiveMeshVertices(id)
            val indices = NativeBridge.getPrimitiveMeshIndices(id)
            val transform = NativeBridge.getPrimitiveTransform(id)
            val color = NativeBridge.getPrimitiveColor(id)
            if (verts != null && indices != null && transform != null && color != null) {
                filamentRenderer.uploadPrimitiveMesh(id, verts, indices, transform, color)
            }
        }
    }

    fun setDrawMode(mode: Int) {
        currentDrawMode = mode
        NativeBridge.setDrawMode(mode)
        stylusInputHandler.setDrawMode(mode)
    }

    @SuppressLint("ClickableViewAccessibility")
    override fun onTouchEvent(event: MotionEvent): Boolean {
        // Try camera first (multi-touch always goes to camera)
        if (cameraController.onTouchEvent(event, isDrawingMode())) {
            return true
        }

        // Then try stylus/drawing input
        if (stylusInputHandler.onTouchEvent(event)) {
            return true
        }

        return super.onTouchEvent(event)
    }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        cameraController.viewWidth = w.toFloat()
        cameraController.viewHeight = h.toFloat()
    }

    fun resume() {
        isRunning = true
        choreographer?.postFrameCallback(this)
    }

    fun pause() {
        isRunning = false
        choreographer?.removeFrameCallback(this)
    }

    override fun doFrame(frameTimeNanos: Long) {
        if (!isRunning) return

        // Update camera
        filamentRenderer.updateCamera(
            cameraController.eyePosition,
            cameraController.targetPosition,
            cameraController.upVector
        )

        // Render
        filamentRenderer.render()

        // Request next frame
        choreographer?.postFrameCallback(this)
    }

    fun clearAll() {
        NativeBridge.clearStrokes()
        filamentRenderer.clearAll()
        updateStats()
    }

    fun destroy() {
        pause()
        viewScope.cancel()
        filamentRenderer.destroy()
    }

    private fun isDrawingMode(): Boolean {
        return currentDrawMode != -1 // Always in some draw mode
    }

    private fun updateStats() {
        val vertices = NativeBridge.getTotalVertexCount()
        val triangles = NativeBridge.getTotalTriangleCount()
        onStatsUpdate?.invoke(vertices, triangles)
    }
}
