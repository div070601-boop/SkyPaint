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

    // ── Stroke Color ────────────────────────────────────────────────────
    @JvmStatic external fun setStrokeColor(r: Float, g: Float, b: Float, a: Float)

    // ── Drafting State ──────────────────────────────────────────────────
    @JvmStatic external fun setStraightLineMode(enable: Boolean)
    @JvmStatic external fun setGridSnap(enable: Boolean, size: Float)
    @JvmStatic external fun setAngleSnap(enable: Boolean, degrees: Float)
    @JvmStatic external fun getCurrentStrokeLength(): Float

    // ── Voxel Sculpting ─────────────────────────────────────────────────
    @JvmStatic external fun initVoxelGrid(
        resolution: Int,
        boundsMin: Float,
        boundsMax: Float
    )
    @JvmStatic external fun beginSculpt()
    @JvmStatic external fun sculptAt(x: Float, y: Float, z: Float, radius: Float, strength: Float)
    @JvmStatic external fun endSculpt()

    // ── Buffer Data access (Mesh) ─────────────────────────────────────────────────────────
    @JvmStatic external fun setLiquifyRadius(radius: Float)
    @JvmStatic external fun setLiquifyStrength(strength: Float)
    @JvmStatic external fun liquifyAt(
        px: Float, py: Float, pz: Float,
        dx: Float, dy: Float, dz: Float
    )

    @JvmStatic external fun decimateVoxelMesh(ratio: Float)

    @JvmStatic external fun getStrokeMeshVertices(index: Int): FloatArray?
    @JvmStatic external fun getStrokeMeshIndices(index: Int): IntArray?
    @JvmStatic external fun getCombinedMeshVertices(): FloatArray?
    @JvmStatic external fun getCombinedMeshIndices(): IntArray?
    @JvmStatic external fun getCurrentStrokeMeshVertices(): FloatArray?
    @JvmStatic external fun getCurrentStrokeMeshIndices(): IntArray?
    @JvmStatic external fun getVoxelMeshVertices(): FloatArray?
    @JvmStatic external fun getVoxelMeshIndices(): IntArray?

    // -- Primitives ------------------------------------------------------
    @JvmStatic external fun addPrimitive(type: Int, transform: FloatArray, r: Float, g: Float, b: Float, a: Float): Int
    @JvmStatic external fun getPrimitiveCount(): Int
    @JvmStatic external fun removePrimitive(index: Int)
    @JvmStatic external fun clearPrimitives()
    @JvmStatic external fun getPrimitiveMeshVertices(index: Int): FloatArray?
    @JvmStatic external fun getPrimitiveMeshIndices(index: Int): IntArray?
    @JvmStatic external fun getPrimitiveTransform(index: Int): FloatArray?
    @JvmStatic external fun getPrimitiveColor(index: Int): FloatArray?

    // -- Selection & Transform -----------------------------------------------
    @JvmStatic external fun pickObjectAt(rayOx: Float, rayOy: Float, rayOz: Float,
                                          rayDx: Float, rayDy: Float, rayDz: Float): Int
    @JvmStatic external fun getSelectedObjectId(): Int
    @JvmStatic external fun deselectAll()
    @JvmStatic external fun transformPrimitive(index: Int, transform: FloatArray)

    // ── Export ───────────────────────────────────────────────────────────
    @JvmStatic external fun exportOBJ(): String
    @JvmStatic external fun exportGLB(): ByteArray?
    @JvmStatic external fun exportOBJToFile(path: String): Boolean
    @JvmStatic external fun exportGLBToFile(path: String): Boolean

    // ── Scene Stats ─────────────────────────────────────────────────────
    @JvmStatic external fun getTotalVertexCount(): Int
    @JvmStatic external fun getTotalTriangleCount(): Int

    // ── Undo / Redo ─────────────────────────────────────────────────────
    @JvmStatic external fun undo()
    @JvmStatic external fun redo()
    @JvmStatic external fun canUndo(): Boolean
    @JvmStatic external fun canRedo(): Boolean
}
