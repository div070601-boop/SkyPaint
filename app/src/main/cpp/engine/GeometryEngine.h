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
#include "Actions.h"
#include "PrimitiveGenerator.h"
#include "SceneObject.h"
#include "RaycastEngine.h"

namespace sky {

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

    // ── Undo / Redo ─────────────────────────────────────────────────────
    void undo();
    void redo();
    bool canUndo() const { return m_actionStack.canUndo(); }
    bool canRedo() const { return m_actionStack.canRedo(); }

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

    const std::vector<StrokePoint>& getStrokePoints(int strokeIndex) const;

    // ── Tube Parameters ─────────────────────────────────────────────────
    void setTubeRadius(float radius);
    void setTubeSegments(int segments);
    void setSplineTension(float tension);

    // ── Stroke Color ────────────────────────────────────────────────────
    void setStrokeColor(float r, float g, float b, float a);

    // ── Drafting State ──────────────────────────────────────────────────
    void setStraightLineMode(bool enable);
    void setGridSnap(bool enable, float size);
    void setAngleSnap(bool enable, float degrees);
    float getCurrentStrokeLength() const;

    // ── Voxel Sculpting ─────────────────────────────────────────────────
    void initVoxelGrid(int resolution, const Vec3& boundsMin, const Vec3& boundsMax);
    void beginSculpt();
    void sculptAt(const Vec3& position, float radius, float strength = 1.0f);
    void endSculpt();
    Mesh getVoxelMesh();
    bool isVoxelGridInitialized() const { return m_voxelGrid.isInitialized(); }

    // ── Liquify Brush ───────────────────────────────────────────────────
    void setLiquifyRadius(float radius);
    void setLiquifyStrength(float strength);
    void liquifyAt(const Vec3& position, const Vec3& direction);

    // ── Mesh Decimation ─────────────────────────────────────────────────
    void decimateVoxelMesh(float ratio);

    // ── Export ───────────────────────────────────────────────────────────
    int addPrimitive(PrimitiveType type, const Mat4& transform, const Vec4& color = Vec4(1.0f));
    const SceneObject* getPrimitive(int index) const;
    int getPrimitiveCount() const;
    void removePrimitive(int index);
    void clearPrimitives();

    // ── Object Properties ───────────────────────────────────────────────
    bool isPrimitiveVisible(int index) const;
    void setPrimitiveVisible(int index, bool visible);
    bool isPrimitiveLocked(int index) const;
    void setPrimitiveLocked(int index, bool locked);
    std::string getPrimitiveName(int index) const;
    void setPrimitiveName(int index, const std::string& name);

    bool isStrokeVisible(int index) const;
    void setStrokeVisible(int index, bool visible);
    bool isStrokeLocked(int index) const;
    void setStrokeLocked(int index, bool locked);
    std::string getStrokeName(int index) const;
    void setStrokeName(int index, const std::string& name);


    std::string exportOBJ() const;
    std::vector<uint8_t> exportGLB() const;
    bool exportOBJToFile(const std::string& path) const;
    bool exportGLBToFile(const std::string& path) const;

    // ── Scene Info ──────────────────────────────────────────────────────
    int getTotalVertexCount() const;
    int getTotalTriangleCount() const;

    // ── Selection & Transform ───────────────────────────────────────────
    int pickObjectAt(float rayOx, float rayOy, float rayOz,
                     float rayDx, float rayDy, float rayDz);
    int getSelectedObjectId() const { return m_selectedObjectId; }
    void setSelectedObjectId(int id) { m_selectedObjectId = id; }
    void deselectAll() { m_selectedObjectId = -1; }
    void transformPrimitive(int index, const Mat4& transform);
    
    // Merge primitive to voxel layer (CSG)
    void mergeSelectedPrimitive(bool subtract);

    // Access stroke meshes for picking
    const std::vector<Mesh>& getStrokeMeshes() const { return m_strokeMeshes; }
    const std::vector<SceneObject>& getPrimitiveObjects() const { return m_primitiveObjects; }
    std::vector<SceneObject>& getPrimitiveObjectsMut() { return m_primitiveObjects; }

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

    ActionStack m_actionStack;
    std::vector<float> m_preSculptSnapshot;

    // Cached stroke meshes
    std::vector<Mesh> m_strokeMeshes;

    // Cached voxel mesh
    Mesh m_voxelMesh;
    bool m_voxelMeshDirty = true;

    std::vector<SceneObject> m_primitiveObjects;
    int m_nextObjectId = 1;
    int m_selectedObjectId = -1;

    bool m_gridSnap = false;
    float m_gridSize = 0.5f;

    /// Generate tube mesh from stroke
    Mesh generateMeshFromStroke(const std::vector<StrokePoint>& points);
};

} // namespace sky
