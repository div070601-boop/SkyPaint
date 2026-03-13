package com.feather3d.app

import android.view.MotionEvent
import android.view.InputDevice

/**
 * Handles stylus and touch input, converting screen events
 * into 3D stroke points via camera ray casting.
 */
class StylusInputHandler(
    private val cameraController: CameraController
) {

    interface Listener {
        fun onStrokeBegin()
        fun onStrokePoint(worldX: Float, worldY: Float, worldZ: Float,
                          pressure: Float, tilt: Float, timestamp: Float)
        fun onStrokeEnd()
        fun onSculptPoint(worldX: Float, worldY: Float, worldZ: Float,
                          pressure: Float)
    }

    var listener: Listener? = null

    // Drawing parameters
    var drawDistance = 2.0f  // depth distance from camera for stroke placement

    private var isStroking = false
    private var currentDrawMode = NativeBridge.MODE_STROKE

    fun setDrawMode(mode: Int) {
        currentDrawMode = mode
    }

    /**
     * Process a touch event for drawing.
     * Returns true if the event was consumed for drawing.
     */
    fun onTouchEvent(event: MotionEvent): Boolean {
        // Only process stylus or primary touch for drawing
        val isStylus = event.getToolType(0) == MotionEvent.TOOL_TYPE_STYLUS
        val isPrimaryTouch = event.pointerCount == 1

        if (!isPrimaryTouch && !isStylus) return false

        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                isStroking = true

                when {
                    isStrokeMode() -> {
                        listener?.onStrokeBegin()
                        processStrokePoint(event)
                    }
                    isSculptMode() || isLiquifyMode() -> {
                        processSculptPoint(event)
                    }
                }
                return true
            }

            MotionEvent.ACTION_MOVE -> {
                if (!isStroking) return false

                // Process historical events for smoother strokes
                for (h in 0 until event.historySize) {
                    val historicalEvent = HistoricalEvent(event, h)
                    when {
                        isStrokeMode() -> processHistoricalStrokePoint(historicalEvent)
                        isSculptMode() || isLiquifyMode() -> processHistoricalSculptPoint(historicalEvent)
                    }
                }

                when {
                    isStrokeMode() -> processStrokePoint(event)
                    isSculptMode() || isLiquifyMode() -> processSculptPoint(event)
                }
                return true
            }

            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                if (isStroking) {
                    if (isStrokeMode()) {
                        listener?.onStrokeEnd()
                    }
                    isStroking = false
                    return true
                }
            }
        }

        return false
    }

    private fun isStrokeMode() = currentDrawMode == NativeBridge.MODE_STROKE
    private fun isSculptMode() = currentDrawMode in listOf(
        NativeBridge.MODE_SCULPT_ADD,
        NativeBridge.MODE_SCULPT_SUB,
        NativeBridge.MODE_SCULPT_SMOOTH
    )
    private fun isLiquifyMode() = currentDrawMode in listOf(
        NativeBridge.MODE_LIQUIFY,
        NativeBridge.MODE_LIQUIFY_SMOOTH,
        NativeBridge.MODE_LIQUIFY_INFLATE,
        NativeBridge.MODE_LIQUIFY_PINCH
    )

    private fun processStrokePoint(event: MotionEvent) {
        val pressure = getPressure(event)
        val tilt = getTilt(event)
        val timestamp = event.eventTime / 1000f

        val worldPos = screenToWorld(event.x, event.y)

        listener?.onStrokePoint(
            worldPos[0], worldPos[1], worldPos[2],
            pressure, tilt, timestamp
        )
    }

    private fun processHistoricalStrokePoint(he: HistoricalEvent) {
        val worldPos = screenToWorld(he.x, he.y)
        listener?.onStrokePoint(
            worldPos[0], worldPos[1], worldPos[2],
            he.pressure, he.tilt, he.timestamp
        )
    }

    private fun processSculptPoint(event: MotionEvent) {
        val pressure = getPressure(event)
        val worldPos = screenToWorld(event.x, event.y)
        listener?.onSculptPoint(worldPos[0], worldPos[1], worldPos[2], pressure)
    }

    private fun processHistoricalSculptPoint(he: HistoricalEvent) {
        val worldPos = screenToWorld(he.x, he.y)
        listener?.onSculptPoint(worldPos[0], worldPos[1], worldPos[2], he.pressure)
    }

    /**
     * Convert screen coordinates to a world-space position.
     * Places the point on a plane perpendicular to the camera view
     * at [drawDistance] from the camera.
     */
    private fun screenToWorld(screenX: Float, screenY: Float): FloatArray {
        val viewMatrix = cameraController.getViewMatrix()
        val projMatrix = cameraController.getProjectionMatrix(
            cameraController.viewWidth / cameraController.viewHeight
        )

        val ray = cameraController.screenToWorldRay(screenX, screenY, viewMatrix, projMatrix)
        val origin = floatArrayOf(ray[0], ray[1], ray[2])
        val dir = floatArrayOf(ray[3], ray[4], ray[5])

        // Place point at draw distance along the ray
        return floatArrayOf(
            origin[0] + dir[0] * drawDistance,
            origin[1] + dir[1] * drawDistance,
            origin[2] + dir[2] * drawDistance
        )
    }

    private fun getPressure(event: MotionEvent): Float {
        var pressure = event.pressure
        // Normalize pressure (some devices report > 1.0)
        pressure = pressure.coerceIn(0f, 1f)
        // If no pressure data, default to 1.0
        if (pressure <= 0f) pressure = 1f
        return pressure
    }

    private fun getTilt(event: MotionEvent): Float {
        return try {
            val axis = event.getAxisValue(MotionEvent.AXIS_TILT, 0)
            axis.coerceIn(0f, Math.PI.toFloat() / 2f)
        } catch (_: Exception) {
            0f
        }
    }

    /** Helper to extract historical event data uniformly */
    private class HistoricalEvent(event: MotionEvent, historyIndex: Int) {
        val x: Float = event.getHistoricalX(historyIndex)
        val y: Float = event.getHistoricalY(historyIndex)
        val pressure: Float = event.getHistoricalPressure(historyIndex).coerceIn(0f, 1f).let {
            if (it <= 0f) 1f else it
        }
        val tilt: Float = try {
            event.getHistoricalAxisValue(MotionEvent.AXIS_TILT, 0, historyIndex)
                .coerceIn(0f, Math.PI.toFloat() / 2f)
        } catch (_: Exception) { 0f }
        val timestamp: Float = event.getHistoricalEventTime(historyIndex) / 1000f
    }
}
