package com.feather3d.app

import android.view.MotionEvent
import kotlin.math.*

/**
 * Orbit/Pan/Zoom camera controller.
 * - Single finger drag: orbit (rotate around target)
 * - Two finger drag: pan
 * - Pinch: zoom
 */
class CameraController {

    // Camera state
    var eyePosition = floatArrayOf(0f, 0f, 3f)
        private set
    var targetPosition = floatArrayOf(0f, 0f, 0f)
        private set
    var upVector = floatArrayOf(0f, 1f, 0f)
        private set

    // Spherical coordinates (relative to target)
    private var azimuth = 0f       // horizontal angle (radians)
    private var elevation = 0.3f   // vertical angle (radians)
    private var distance = 3f      // distance from target

    // Orbit sensitivity
    var orbitSensitivity = 0.005f
    var panSensitivity = 0.003f
    var zoomSensitivity = 1.0f
    var minDistance = 0.1f
    var maxDistance = 50f
    var minElevation = -PI.toFloat() / 2f + 0.01f
    var maxElevation = PI.toFloat() / 2f - 0.01f

    // Touch tracking
    private var previousX = 0f
    private var previousY = 0f
    private var previousSpacing = 0f
    private var previousMidX = 0f
    private var previousMidY = 0f
    private var pointerCount = 0
    private var isDragging = false

    // View dimensions (for pan calculation)
    var viewWidth = 1f
    var viewHeight = 1f

    init {
        updateEyePosition()
    }

    /**
     * Process touch events for camera manipulation.
     * Returns true if the event was consumed (camera interaction),
     * false if it should be passed to the drawing system.
     */
    fun onTouchEvent(event: MotionEvent, isDrawMode: Boolean): Boolean {
        // In draw mode, only respond to multi-touch (2+ fingers)
        if (isDrawMode && event.pointerCount < 2) {
            return false
        }

        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                previousX = event.x
                previousY = event.y
                pointerCount = 1
                isDragging = false
                return !isDrawMode
            }

            MotionEvent.ACTION_POINTER_DOWN -> {
                pointerCount = event.pointerCount
                if (pointerCount >= 2) {
                    previousSpacing = getSpacing(event)
                    val mid = getMidpoint(event)
                    previousMidX = mid[0]
                    previousMidY = mid[1]
                    isDragging = true
                }
                return true
            }

            MotionEvent.ACTION_MOVE -> {
                if (pointerCount >= 2 && event.pointerCount >= 2) {
                    // Two-finger: pan + pinch zoom
                    val spacing = getSpacing(event)
                    val mid = getMidpoint(event)

                    // Pinch zoom
                    if (previousSpacing > 10f && spacing > 10f) {
                        val scale = previousSpacing / spacing
                        distance *= scale
                        distance = distance.coerceIn(minDistance, maxDistance)
                    }

                    // Pan
                    val dx = mid[0] - previousMidX
                    val dy = mid[1] - previousMidY
                    pan(dx, dy)

                    previousSpacing = spacing
                    previousMidX = mid[0]
                    previousMidY = mid[1]

                    updateEyePosition()
                    return true
                } else if (pointerCount == 1 && !isDrawMode) {
                    // Single finger: orbit
                    val dx = event.x - previousX
                    val dy = event.y - previousY

                    if (abs(dx) > 1f || abs(dy) > 1f) {
                        isDragging = true
                    }

                    azimuth -= dx * orbitSensitivity
                    elevation += dy * orbitSensitivity
                    elevation = elevation.coerceIn(minElevation, maxElevation)

                    previousX = event.x
                    previousY = event.y

                    updateEyePosition()
                    return true
                }
            }

            MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP -> {
                pointerCount = event.pointerCount - 1
                if (pointerCount < 2) {
                    if (event.pointerCount > 0) {
                        previousX = event.getX(0)
                        previousY = event.getY(0)
                    }
                }
                return isDragging
            }

            MotionEvent.ACTION_CANCEL -> {
                pointerCount = 0
                isDragging = false
                return true
            }
        }

        return false
    }

    /** Zoom by delta (positive = zoom in, negative = zoom out) */
    fun zoom(delta: Float) {
        distance -= delta * zoomSensitivity
        distance = distance.coerceIn(minDistance, maxDistance)
        updateEyePosition()
    }

    /** Reset camera to default position */
    fun reset() {
        azimuth = 0f
        elevation = 0.3f
        distance = 3f
        targetPosition = floatArrayOf(0f, 0f, 0f)
        updateEyePosition()
    }

    /** Get the view matrix as a column-major 4x4 float array */
    fun getViewMatrix(): FloatArray {
        return lookAt(eyePosition, targetPosition, upVector)
    }

    /** Get the projection matrix */
    fun getProjectionMatrix(aspectRatio: Float, fovDegrees: Float = 45f,
                            near: Float = 0.01f, far: Float = 100f): FloatArray {
        return perspective(fovDegrees, aspectRatio, near, far)
    }

    /**
     * Unproject a screen point to a world-space ray.
     * Returns [originX, originY, originZ, dirX, dirY, dirZ]
     */
    fun screenToWorldRay(screenX: Float, screenY: Float,
                         viewMatrix: FloatArray, projMatrix: FloatArray): FloatArray {
        // Normalize screen coords to [-1, 1]
        val ndcX = (2f * screenX / viewWidth) - 1f
        val ndcY = 1f - (2f * screenY / viewHeight)

        // Inverse VP matrix
        val vp = multiplyMat4(projMatrix, viewMatrix)
        val invVP = invertMat4(vp) ?: return floatArrayOf(0f, 0f, 0f, 0f, 0f, -1f)

        // Near and far points
        val nearPoint = multiplyMat4Vec4(invVP, floatArrayOf(ndcX, ndcY, -1f, 1f))
        val farPoint = multiplyMat4Vec4(invVP, floatArrayOf(ndcX, ndcY, 1f, 1f))

        // Perspective divide
        val near = floatArrayOf(nearPoint[0]/nearPoint[3], nearPoint[1]/nearPoint[3], nearPoint[2]/nearPoint[3])
        val far = floatArrayOf(farPoint[0]/farPoint[3], farPoint[1]/farPoint[3], farPoint[2]/farPoint[3])

        // Direction
        val dx = far[0] - near[0]
        val dy = far[1] - near[1]
        val dz = far[2] - near[2]
        val len = sqrt(dx*dx + dy*dy + dz*dz)

        return floatArrayOf(near[0], near[1], near[2], dx/len, dy/len, dz/len)
    }

    // ── Private helpers ─────────────────────────────────────────────────

    private fun updateEyePosition() {
        val cosE = cos(elevation)
        eyePosition[0] = targetPosition[0] + distance * cosE * sin(azimuth)
        eyePosition[1] = targetPosition[1] + distance * sin(elevation)
        eyePosition[2] = targetPosition[2] + distance * cosE * cos(azimuth)
    }

    private fun pan(dx: Float, dy: Float) {
        // Calculate camera right and up vectors in world space
        val forward = floatArrayOf(
            targetPosition[0] - eyePosition[0],
            targetPosition[1] - eyePosition[1],
            targetPosition[2] - eyePosition[2]
        )
        val fLen = sqrt(forward[0]*forward[0] + forward[1]*forward[1] + forward[2]*forward[2])
        forward[0] /= fLen; forward[1] /= fLen; forward[2] /= fLen

        val right = cross(forward, upVector)
        val rLen = sqrt(right[0]*right[0] + right[1]*right[1] + right[2]*right[2])
        right[0] /= rLen; right[1] /= rLen; right[2] /= rLen

        val camUp = cross(right, forward)

        val panScale = distance * panSensitivity
        val offsetX = -dx * panScale
        val offsetY = dy * panScale

        targetPosition[0] += right[0] * offsetX + camUp[0] * offsetY
        targetPosition[1] += right[1] * offsetX + camUp[1] * offsetY
        targetPosition[2] += right[2] * offsetX + camUp[2] * offsetY
    }

    private fun getSpacing(event: MotionEvent): Float {
        if (event.pointerCount < 2) return 0f
        val dx = event.getX(0) - event.getX(1)
        val dy = event.getY(0) - event.getY(1)
        return sqrt(dx * dx + dy * dy)
    }

    private fun getMidpoint(event: MotionEvent): FloatArray {
        if (event.pointerCount < 2) return floatArrayOf(event.x, event.y)
        return floatArrayOf(
            (event.getX(0) + event.getX(1)) / 2f,
            (event.getY(0) + event.getY(1)) / 2f
        )
    }

    private fun cross(a: FloatArray, b: FloatArray): FloatArray {
        return floatArrayOf(
            a[1]*b[2] - a[2]*b[1],
            a[2]*b[0] - a[0]*b[2],
            a[0]*b[1] - a[1]*b[0]
        )
    }

    companion object {
        /** Build a look-at view matrix (column-major) */
        fun lookAt(eye: FloatArray, target: FloatArray, up: FloatArray): FloatArray {
            val fx = target[0] - eye[0]
            val fy = target[1] - eye[1]
            val fz = target[2] - eye[2]
            val fLen = sqrt(fx*fx + fy*fy + fz*fz)
            val f = floatArrayOf(fx/fLen, fy/fLen, fz/fLen)

            // right = f × up
            val rx = f[1]*up[2] - f[2]*up[1]
            val ry = f[2]*up[0] - f[0]*up[2]
            val rz = f[0]*up[1] - f[1]*up[0]
            val rLen = sqrt(rx*rx + ry*ry + rz*rz)
            val r = floatArrayOf(rx/rLen, ry/rLen, rz/rLen)

            // u = right × f
            val u = floatArrayOf(
                r[1]*f[2] - r[2]*f[1],
                r[2]*f[0] - r[0]*f[2],
                r[0]*f[1] - r[1]*f[0]
            )

            // Column-major 4x4
            return floatArrayOf(
                r[0], u[0], -f[0], 0f,
                r[1], u[1], -f[1], 0f,
                r[2], u[2], -f[2], 0f,
                -(r[0]*eye[0] + r[1]*eye[1] + r[2]*eye[2]),
                -(u[0]*eye[0] + u[1]*eye[1] + u[2]*eye[2]),
                (f[0]*eye[0] + f[1]*eye[1] + f[2]*eye[2]),
                1f
            )
        }

        /** Build a perspective projection matrix (column-major) */
        fun perspective(fovDegrees: Float, aspect: Float, near: Float, far: Float): FloatArray {
            val fovRad = fovDegrees * PI.toFloat() / 180f
            val tanHalf = tan(fovRad / 2f)
            val range = far - near

            return floatArrayOf(
                1f / (aspect * tanHalf), 0f, 0f, 0f,
                0f, 1f / tanHalf, 0f, 0f,
                0f, 0f, -(far + near) / range, -1f,
                0f, 0f, -2f * far * near / range, 0f
            )
        }

        fun multiplyMat4(a: FloatArray, b: FloatArray): FloatArray {
            val result = FloatArray(16)
            for (col in 0..3) {
                for (row in 0..3) {
                    var sum = 0f
                    for (k in 0..3) {
                        sum += a[k * 4 + row] * b[col * 4 + k]
                    }
                    result[col * 4 + row] = sum
                }
            }
            return result
        }

        fun multiplyMat4Vec4(m: FloatArray, v: FloatArray): FloatArray {
            return floatArrayOf(
                m[0]*v[0] + m[4]*v[1] + m[8]*v[2] + m[12]*v[3],
                m[1]*v[0] + m[5]*v[1] + m[9]*v[2] + m[13]*v[3],
                m[2]*v[0] + m[6]*v[1] + m[10]*v[2] + m[14]*v[3],
                m[3]*v[0] + m[7]*v[1] + m[11]*v[2] + m[15]*v[3]
            )
        }

        fun invertMat4(m: FloatArray): FloatArray? {
            val inv = FloatArray(16)
            inv[0] = m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15] + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10]
            inv[4] = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15] - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10]
            inv[8] = m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15] + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9]
            inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14] - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9]
            inv[1] = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15] - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10]
            inv[5] = m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15] + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10]
            inv[9] = -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15] - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9]
            inv[13] = m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14] + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9]
            inv[2] = m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15] + m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6]
            inv[6] = -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15] - m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6]
            inv[10] = m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15] + m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5]
            inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14] - m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5]
            inv[3] = -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11] - m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6]
            inv[7] = m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11] + m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6]
            inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11] - m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5]
            inv[15] = m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10] + m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5]

            val det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12]
            if (abs(det) < 1e-10f) return null

            val invDet = 1f / det
            for (i in 0..15) inv[i] *= invDet
            return inv
        }
    }
}
