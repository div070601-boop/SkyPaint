package com.feather3d.app

/**
 * JNI bridge to the native C++ GeometryEngine.
 * All methods are static and operate on a singleton engine instance.
 */
object NativeBridge {

    init {
        System.loadLibrary("feather3d_engine")
    }

    // ── Engine Lifecycle ────────────────────────────────────────────────
    @JvmStatic external fun nativeInit()
    @JvmStatic external fun nativeDestroy()

    // ── Draw Mode ───────────────────────────────────────────────────────
    @JvmStatic external fun setDrawMode(mode: Int)
    @JvmStatic external fun getDrawMode(): Int

    // Draw mode constants
    const val MODE_STROKE = 0
    const val MODE_SCULPT_ADD = 1
    const val MODE_SCULPT_SUB = 2
    const val MODE_SCULPT_SMOOTH = 3
    const val MODE_LIQUIFY = 4
    const val MODE_LIQUIFY_SMOOTH = 5
    const val MODE_LIQUIFY_INFLATE = 6
    const val MODE_LIQUIFY_PINCH = 7

    // ── Stroke Drawing ──────────────────────────────────────────────────
    @JvmStatic external fun beginStroke()
    @JvmStatic external fun addStrokePoint(
        x: Float, y: Float, z: Float,
        pressure: Float, tilt: Float, timestamp: Float
    )
    @JvmStatic external fun endStroke(): Int
    @JvmStatic external fun getStrokeCount(): Int
    @JvmStatic external fun removeStroke(index: Int)
    @JvmStatic external fun clearStrokes()

    // ── Tube Parameters ─────────────────────────────────────────────────
    @JvmStatic external fun setTubeRadius(radius: Float)
    @JvmStatic external fun setTubeSegments(segments: Int)

    // ── Voxel Sculpting ─────────────────────────────────────────────────
    @JvmStatic external fun initVoxelGrid(
        resolution: Int,
        minX: Float, minY: Float, minZ: Float,
        maxX: Float, maxY: Float, maxZ: Float
    )
    @JvmStatic external fun sculptAt(
        x: Float, y: Float, z: Float,
        radius: Float, strength: Float
    )

    // ── Liquify ─────────────────────────────────────────────────────────
    @JvmStatic external fun setLiquifyRadius(radius: Float)
    @JvmStatic external fun setLiquifyStrength(strength: Float)
    @JvmStatic external fun liquifyAt(
        px: Float, py: Float, pz: Float,
        dx: Float, dy: Float, dz: Float
    )

    // ── Decimation ──────────────────────────────────────────────────────
    @JvmStatic external fun decimateVoxelMesh(ratio: Float)

    // ── Mesh Data (for renderer) ────────────────────────────────────────
    @JvmStatic external fun getStrokeMeshVertices(index: Int): FloatArray?
    @JvmStatic external fun getStrokeMeshIndices(index: Int): IntArray?
    @JvmStatic external fun getCombinedMeshVertices(): FloatArray?
    @JvmStatic external fun getCombinedMeshIndices(): IntArray?
    @JvmStatic external fun getCurrentStrokeMeshVertices(): FloatArray?
    @JvmStatic external fun getCurrentStrokeMeshIndices(): IntArray?
    @JvmStatic external fun getVoxelMeshVertices(): FloatArray?
    @JvmStatic external fun getVoxelMeshIndices(): IntArray?

    // ── Export ───────────────────────────────────────────────────────────
    @JvmStatic external fun exportOBJ(): String
    @JvmStatic external fun exportGLB(): ByteArray?
    @JvmStatic external fun exportOBJToFile(path: String): Boolean
    @JvmStatic external fun exportGLBToFile(path: String): Boolean

    // ── Scene Info ──────────────────────────────────────────────────────
    @JvmStatic external fun getTotalVertexCount(): Int
    @JvmStatic external fun getTotalTriangleCount(): Int
}
