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

    // ── Primitives ──────────────────────────────────────────────────────────

    fun addPrimitive(type: Int, r: Float = 0.5f, g: Float = 0.5f, b: Float = 0.5f) {
        // Spawn at camera's look-at target so it appears in view
        val target = cameraController.targetPosition
        val transform = floatArrayOf(
            1f, 0f, 0f, 0f,
            0f, 1f, 0f, 0f,
            0f, 0f, 1f, 0f,
            target[0], target[1], target[2], 1f
        )
        val id = NativeBridge.addPrimitive(type, transform, r, g, b, 1.0f)
        if (id >= 0) {
            val verts = NativeBridge.getPrimitiveMeshVertices(id)
            val indices = NativeBridge.getPrimitiveMeshIndices(id)
            val t = NativeBridge.getPrimitiveTransform(id)
            val color = NativeBridge.getPrimitiveColor(id)
            if (verts != null && indices != null && t != null && color != null) {
                filamentRenderer.uploadPrimitiveMesh(id, verts, indices, t, color)
            }
        }
    }

    // ── Selection ───────────────────────────────────────────────────────────

    fun deleteSelected() {
        val selectedId = NativeBridge.getSelectedObjectId()
        if (selectedId >= 0) {
            val count = NativeBridge.getPrimitiveCount()
            for (i in 0 until count) {
                NativeBridge.removePrimitive(i)
                filamentRenderer.clearAll()
                refreshAllPrimitives()
                refreshAllStrokes()
                break
            }
            NativeBridge.deselectAll()
            onSelectionChanged?.invoke(-1)
        }
    }

    fun duplicateSelected() {
        val selectedId = NativeBridge.getSelectedObjectId()
        if (selectedId >= 0) {
            val count = NativeBridge.getPrimitiveCount()
            for (i in 0 until count) {
                val transform = NativeBridge.getPrimitiveTransform(i)
                val color = NativeBridge.getPrimitiveColor(i)
                if (transform != null && color != null) {
                    // Offset the duplicate slightly
                    val newTransform = transform.copyOf()
                    newTransform[12] += 0.3f // offset X

                    // Re-add as a new primitive (type 0=cube is a placeholder;
                    // the mesh is what matters, and it will be the same type)
                    val newId = NativeBridge.addPrimitive(0, newTransform,
                        color[0], color[1], color[2], color[3])
                    if (newId >= 0) {
                        val verts = NativeBridge.getPrimitiveMeshVertices(newId)
                        val indices = NativeBridge.getPrimitiveMeshIndices(newId)
                        val t = NativeBridge.getPrimitiveTransform(newId)
                        val c = NativeBridge.getPrimitiveColor(newId)
                        if (verts != null && indices != null && t != null && c != null) {
                            filamentRenderer.uploadPrimitiveMesh(newId, verts, indices, t, c)
                        }
                    }
                    break
                }
            }
        }
    }

    private fun refreshAllPrimitives() {
        val count = NativeBridge.getPrimitiveCount()
        for (i in 0 until count) {
            val verts = NativeBridge.getPrimitiveMeshVertices(i)
            val indices = NativeBridge.getPrimitiveMeshIndices(i)
            val transform = NativeBridge.getPrimitiveTransform(i)
            val color = NativeBridge.getPrimitiveColor(i)
            if (verts != null && indices != null && transform != null && color != null) {
                filamentRenderer.uploadPrimitiveMesh(i, verts, indices, transform, color)
            }
        }
    }

    private fun refreshAllStrokes() {
        val strokeCount = NativeBridge.getStrokeCount()
        for (i in 0 until strokeCount) {
            val verts = NativeBridge.getStrokeMeshVertices(i)
            val indices = NativeBridge.getStrokeMeshIndices(i)
            if (verts != null && indices != null) {
                filamentRenderer.uploadStrokeMesh(i, verts, indices)
            }
        }
        val voxVerts = NativeBridge.getVoxelMeshVertices()
        val voxIdx = NativeBridge.getVoxelMeshIndices()
        if (voxVerts != null && voxIdx != null && voxVerts.isNotEmpty()) {
            filamentRenderer.uploadVoxelMesh(voxVerts, voxIdx)
        }
    }

    // ── Drawing Mode ────────────────────────────────────────────────────────

    fun setDrawMode(mode: Int) {
        currentDrawMode = mode
        if (mode >= 0) {
            NativeBridge.setDrawMode(mode)
        }
        stylusInputHandler.setDrawMode(mode)
    }

    // Selection state
    private var selectDragStartX = 0f
    private var selectDragStartY = 0f
    private var isDraggingSelected = false
    private var lastPinchDist = 0f
    private var lastPinchAngle = 0f
    var onSelectionChanged: ((Int) -> Unit)? = null

    @SuppressLint("ClickableViewAccessibility")
    override fun onTouchEvent(event: MotionEvent): Boolean {
        // Select mode: mode == -2
        if (currentDrawMode == -2 && event.pointerCount == 1) {
            when (event.actionMasked) {
                MotionEvent.ACTION_DOWN -> {
                    selectDragStartX = event.x
                    selectDragStartY = event.y
                    isDraggingSelected = false

                    // Fire a pick ray
                    val viewMatrix = cameraController.getViewMatrix()
                    val projMatrix = cameraController.getProjectionMatrix(
                        cameraController.viewWidth / cameraController.viewHeight
                    )
                    val ray = cameraController.screenToWorldRay(event.x, event.y, viewMatrix, projMatrix)
                    val hitId = NativeBridge.pickObjectAt(
                        ray[0], ray[1], ray[2],  // origin
                        ray[3], ray[4], ray[5]   // direction
                    )
                    onSelectionChanged?.invoke(hitId)
                    return true
                }
                MotionEvent.ACTION_MOVE -> {
                    val selectedId = NativeBridge.getSelectedObjectId()
                    if (selectedId >= 0) { // Only primitives can be moved (positive IDs)
                        val dx = event.x - selectDragStartX
                        val dy = event.y - selectDragStartY
                        if (!isDraggingSelected && (dx * dx + dy * dy) > 100f) {
                            isDraggingSelected = true
                        }
                        if (isDraggingSelected) {
                            // Convert screen delta to world delta
                            val scale = stylusInputHandler.drawDistance / cameraController.viewHeight * 2f
                            val worldDx = dx * scale
                            val worldDy = -dy * scale // Y is inverted

                            // Find the primitive index by searching for matching ID
                            val count = NativeBridge.getPrimitiveCount()
                            for (i in 0 until count) {
                                val transform = NativeBridge.getPrimitiveTransform(i)
                                if (transform != null) {
                                    // Apply translation delta to current transform
                                    val newTransform = transform.copyOf()
                                    newTransform[12] += worldDx  // translate X
                                    newTransform[13] += worldDy  // translate Y
                                    NativeBridge.transformPrimitive(i, newTransform)

                                    // Re-upload mesh with new transform
                                    val verts = NativeBridge.getPrimitiveMeshVertices(i)
                                    val indices = NativeBridge.getPrimitiveMeshIndices(i)
                                    val color = NativeBridge.getPrimitiveColor(i)
                                    if (verts != null && indices != null && color != null) {
                                        filamentRenderer.uploadPrimitiveMesh(i, verts, indices, newTransform, color)
                                    }
                                    break
                                }
                            }
                            selectDragStartX = event.x
                            selectDragStartY = event.y
                        }
                    }
                    return true
                }
                MotionEvent.ACTION_UP -> {
                    isDraggingSelected = false
                    return true
                }
            }
        }

        // Pinch-to-scale and rotate in Select mode (2 fingers)
        if (currentDrawMode == -2 && event.pointerCount == 2) {
            val selectedId = NativeBridge.getSelectedObjectId()
            if (selectedId >= 0) {
                when (event.actionMasked) {
                    MotionEvent.ACTION_POINTER_DOWN -> {
                        val dx = event.getX(0) - event.getX(1)
                        val dy = event.getY(0) - event.getY(1)
                        lastPinchDist = Math.sqrt((dx * dx + dy * dy).toDouble()).toFloat()
                        lastPinchAngle = Math.atan2(dy.toDouble(), dx.toDouble()).toFloat()
                        return true
                    }
                    MotionEvent.ACTION_MOVE -> {
                        val dx = event.getX(0) - event.getX(1)
                        val dy = event.getY(0) - event.getY(1)
                        val dist = Math.sqrt((dx * dx + dy * dy).toDouble()).toFloat()
                        val angle = Math.atan2(dy.toDouble(), dx.toDouble()).toFloat()

                        if (lastPinchDist > 0f) {
                            val scaleFactor = dist / lastPinchDist
                            val rotDelta = angle - lastPinchAngle

                            val count = NativeBridge.getPrimitiveCount()
                            for (i in 0 until count) {
                                val transform = NativeBridge.getPrimitiveTransform(i)
                                if (transform != null) {
                                    val t = transform.copyOf()

                                    // Apply uniform scale
                                    for (col in 0..2) {
                                        t[col * 4 + 0] *= scaleFactor
                                        t[col * 4 + 1] *= scaleFactor
                                        t[col * 4 + 2] *= scaleFactor
                                    }

                                    // Apply Y-axis rotation
                                    if (Math.abs(rotDelta) > 0.001f) {
                                        val cosR = Math.cos(rotDelta.toDouble()).toFloat()
                                        val sinR = Math.sin(rotDelta.toDouble()).toFloat()
                                        // Rotate columns 0 and 2 (X and Z axes) around Y
                                        val c0x = t[0]; val c0y = t[1]; val c0z = t[2]
                                        val c2x = t[8]; val c2y = t[9]; val c2z = t[10]
                                        t[0] = c0x * cosR + c2x * sinR
                                        t[1] = c0y * cosR + c2y * sinR
                                        t[2] = c0z * cosR + c2z * sinR
                                        t[8] = -c0x * sinR + c2x * cosR
                                        t[9] = -c0y * sinR + c2y * cosR
                                        t[10] = -c0z * sinR + c2z * cosR
                                    }

                                    NativeBridge.transformPrimitive(i, t)
                                    val verts = NativeBridge.getPrimitiveMeshVertices(i)
                                    val indices = NativeBridge.getPrimitiveMeshIndices(i)
                                    val color = NativeBridge.getPrimitiveColor(i)
                                    if (verts != null && indices != null && color != null) {
                                        filamentRenderer.uploadPrimitiveMesh(i, verts, indices, t, color)
                                    }
                                    break
                                }
                            }
                        }
                        lastPinchDist = dist
                        lastPinchAngle = angle
                        return true
                    }
                    MotionEvent.ACTION_POINTER_UP -> {
                        lastPinchDist = 0f
                        lastPinchAngle = 0f
                        return true
                    }
                }
            }
        }

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
        NativeBridge.clearPrimitives()
        NativeBridge.deselectAll()
        filamentRenderer.clearAll()
        updateStats()
    }

    fun destroy() {
        pause()
        viewScope.cancel()
        filamentRenderer.destroy()
    }

    private fun isDrawingMode(): Boolean {
        return currentDrawMode >= 0 // Navigate (-1) and Select (-2) are not drawing modes
    }

    private fun updateStats() {
        val vertices = NativeBridge.getTotalVertexCount()
        val triangles = NativeBridge.getTotalTriangleCount()
        onStatsUpdate?.invoke(vertices, triangles)
    }
}
