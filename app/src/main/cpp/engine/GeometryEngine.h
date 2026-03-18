#pragma once

#include "../math/MathTypes.h"
#include "StrokeSystem.h"
#include "SplineGenerator.h"
#include "TubeMeshGenerator.h"
#include "VoxelGrid.h"
#include "MarchingCubes.h"
#include "LiquifyBrush.h"
#include "MeshDecimation.h"
#include "MeshExporter.h"

namespace feather {

/// Drawing mode for the engine
enum class DrawMode {
    STROKE,     // Stroke-based drawing (tubes)
    SCULPT_ADD, // Voxel sculpt: add material
    SCULPT_SUB, // Voxel sculpt: subtract material
    SCULPT_SMOOTH,
    LIQUIFY,
    LIQUIFY_SMOOTH,
    LIQUIFY_INFLATE,
    LIQUIFY_PINCH,
};

/// Main facade coordinating all geometry subsystems
class GeometryEngine {
public:
    GeometryEngine();
    ~GeometryEngine() = default;

    // ── Drawing Mode ────────────────────────────────────────────────────
    void setDrawMode(DrawMode mode) { m_drawMode = mode; }
    DrawMode getDrawMode() const { return m_drawMode; }

    // ── Stroke Drawing ──────────────────────────────────────────────────
    void beginStroke();
    void addStrokePoint(const Vec3& position, float pressure, float tilt, float timestamp);
    int endStroke();

    /// Get the mesh for a specific stroke
    const Mesh& getStrokeMesh(int strokeIndex) const;

    /// Get all stroke meshes combined
    Mesh getCombinedStrokeMesh() const;

    /// Get the current in-progress stroke mesh (for live preview)
    Mesh getCurrentStrokeMesh() const;

    int getStrokeCount() const;
    void removeStroke(int index);
    void clearStrokes();

    // ── Tube Parameters ─────────────────────────────────────────────────
    void setTubeRadius(float radius);
    void setTubeSegments(int segments);
    void setSplineTension(float tension);

    // ── Stroke Color ────────────────────────────────────────────────────
    void setStrokeColor(float r, float g, float b, float a);

    // ── Voxel Sculpting ─────────────────────────────────────────────────
    void initVoxelGrid(int resolution, const Vec3& boundsMin, const Vec3& boundsMax);
    void sculptAt(const Vec3& position, float radius, float strength = 1.0f);
    Mesh getVoxelMesh();
    bool isVoxelGridInitialized() const { return m_voxelGrid.isInitialized(); }

    // ── Liquify Brush ───────────────────────────────────────────────────
    void setLiquifyRadius(float radius);
    void setLiquifyStrength(float strength);
    void liquifyAt(const Vec3& position, const Vec3& direction);

    // ── Mesh Decimation ─────────────────────────────────────────────────
    void decimateVoxelMesh(float ratio);

    // ── Export ───────────────────────────────────────────────────────────
    std::string exportOBJ() const;
    std::vector<uint8_t> exportGLB() const;
    bool exportOBJToFile(const std::string& path) const;
    bool exportGLBToFile(const std::string& path) const;

    // ── Scene Info ──────────────────────────────────────────────────────
    int getTotalVertexCount() const;
    int getTotalTriangleCount() const;

private:
    DrawMode m_drawMode = DrawMode::STROKE;

    StrokeSystem m_strokeSystem;
    SplineGenerator m_splineGenerator;
    TubeMeshGenerator m_tubeMeshGenerator;
    VoxelGrid m_voxelGrid;
    MarchingCubes m_marchingCubes;
    LiquifyBrush m_liquifyBrush;
    MeshDecimation m_meshDecimation;
    MeshExporter m_meshExporter;

    // Cached stroke meshes
    std::vector<Mesh> m_strokeMeshes;

    // Cached voxel mesh
    Mesh m_voxelMesh;
    bool m_voxelMeshDirty = true;

    /// Generate tube mesh from stroke
    Mesh generateMeshFromStroke(const std::vector<StrokePoint>& points);
};

} // namespace feather
