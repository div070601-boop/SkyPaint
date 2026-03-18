#include "GeometryEngine.h"

namespace feather {

GeometryEngine::GeometryEngine() {
    // Default tube parameters
    m_tubeMeshGenerator.setBaseRadius(0.005f);
    m_tubeMeshGenerator.setRadialSegments(12);
    m_splineGenerator.setTension(0.5f);

    // Default liquify parameters
    m_liquifyBrush.setRadius(0.05f);
    m_liquifyBrush.setStrength(0.5f);
}

// ── Stroke Drawing ──────────────────────────────────────────────────────────

void GeometryEngine::beginStroke() {
    m_strokeSystem.beginStroke();
}

void GeometryEngine::addStrokePoint(const Vec3& position, float pressure,
                                     float tilt, float timestamp) {
    StrokePoint point;
    point.position = position;
    point.pressure = pressure;
    point.tilt = tilt;
    point.timestamp = timestamp;
    m_strokeSystem.addPoint(point);
}

int GeometryEngine::endStroke() {
    int index = m_strokeSystem.endStroke();
    if (index >= 0) {
        const auto& points = m_strokeSystem.getStrokePoints(index);
        Mesh mesh = generateMeshFromStroke(points);

        if (index < static_cast<int>(m_strokeMeshes.size())) {
            m_strokeMeshes[index] = std::move(mesh);
        } else {
            m_strokeMeshes.push_back(std::move(mesh));
        }
    }
    return index;
}

const Mesh& GeometryEngine::getStrokeMesh(int strokeIndex) const {
    static const Mesh empty;
    if (strokeIndex < 0 || strokeIndex >= static_cast<int>(m_strokeMeshes.size())) {
        return empty;
    }
    return m_strokeMeshes[strokeIndex];
}

Mesh GeometryEngine::getCombinedStrokeMesh() const {
    Mesh combined;
    for (const auto& mesh : m_strokeMeshes) {
        uint32_t offset = static_cast<uint32_t>(combined.vertices.size());
        combined.vertices.insert(combined.vertices.end(),
                                 mesh.vertices.begin(), mesh.vertices.end());
        for (uint32_t idx : mesh.indices) {
            combined.indices.push_back(idx + offset);
        }
    }
    return combined;
}

Mesh GeometryEngine::getCurrentStrokeMesh() const {
    const auto& points = m_strokeSystem.getCurrentStrokePoints();
    if (points.size() < 2) return Mesh();

    auto samples = m_splineGenerator.generate(points, 4); // fewer samples for live preview
    return m_tubeMeshGenerator.generate(samples);
}

int GeometryEngine::getStrokeCount() const {
    return m_strokeSystem.getStrokeCount();
}

void GeometryEngine::removeStroke(int index) {
    m_strokeSystem.removeStroke(index);
    if (index >= 0 && index < static_cast<int>(m_strokeMeshes.size())) {
        m_strokeMeshes.erase(m_strokeMeshes.begin() + index);
    }
}

void GeometryEngine::clearStrokes() {
    m_strokeSystem.clearAll();
    m_strokeMeshes.clear();
}

// ── Tube Parameters ─────────────────────────────────────────────────────────

void GeometryEngine::setTubeRadius(float radius) {
    m_tubeMeshGenerator.setBaseRadius(radius);
}

void GeometryEngine::setTubeSegments(int segments) {
    m_tubeMeshGenerator.setRadialSegments(segments);
}

void GeometryEngine::setSplineTension(float tension) {
    m_splineGenerator.setTension(tension);
}

void GeometryEngine::setStrokeColor(float r, float g, float b, float a) {
    m_strokeSystem.setActiveColor(Vec4(r, g, b, a));
}

// ── Voxel Sculpting ─────────────────────────────────────────────────────────

void GeometryEngine::initVoxelGrid(int resolution, const Vec3& boundsMin,
                                    const Vec3& boundsMax) {
    m_voxelGrid.initialize(resolution, boundsMin, boundsMax);
    m_voxelMeshDirty = true;
}

void GeometryEngine::sculptAt(const Vec3& position, float radius, float strength) {
    switch (m_drawMode) {
        case DrawMode::SCULPT_ADD:
            m_voxelGrid.addSphere(position, radius, strength);
            break;
        case DrawMode::SCULPT_SUB:
            m_voxelGrid.subtractSphere(position, radius, strength);
            break;
        case DrawMode::SCULPT_SMOOTH:
            m_voxelGrid.smooth(position, radius, 2);
            break;
        default:
            m_voxelGrid.addSphere(position, radius, strength);
            break;
    }
    m_voxelMeshDirty = true;
}

Mesh GeometryEngine::getVoxelMesh() {
    if (m_voxelMeshDirty && m_voxelGrid.isInitialized()) {
        int xMin, yMin, zMin, xMax, yMax, zMax;
        if (m_voxelGrid.getDirtyBounds(xMin, yMin, zMin, xMax, yMax, zMax)) {
            // Partial update
            m_voxelMesh = m_marchingCubes.extract(m_voxelGrid);
            m_voxelGrid.clearDirty();
        } else {
            m_voxelMesh = m_marchingCubes.extract(m_voxelGrid);
        }
        m_voxelMeshDirty = false;
    }
    return m_voxelMesh;
}

// ── Liquify Brush ───────────────────────────────────────────────────────────

void GeometryEngine::setLiquifyRadius(float radius) {
    m_liquifyBrush.setRadius(radius);
}

void GeometryEngine::setLiquifyStrength(float strength) {
    m_liquifyBrush.setStrength(strength);
}

void GeometryEngine::liquifyAt(const Vec3& position, const Vec3& direction) {
    // Apply to voxel mesh if available
    if (m_voxelGrid.isInitialized()) {
        switch (m_drawMode) {
            case DrawMode::LIQUIFY:
                m_liquifyBrush.apply(m_voxelMesh, position, direction);
                break;
            case DrawMode::LIQUIFY_SMOOTH:
                m_liquifyBrush.smooth(m_voxelMesh, position, 2);
                break;
            case DrawMode::LIQUIFY_INFLATE:
                m_liquifyBrush.inflate(m_voxelMesh, position, 0.01f);
                break;
            case DrawMode::LIQUIFY_PINCH:
                m_liquifyBrush.pinch(m_voxelMesh, position);
                break;
            default:
                m_liquifyBrush.apply(m_voxelMesh, position, direction);
                break;
        }
    }
}

// ── Mesh Decimation ─────────────────────────────────────────────────────────

void GeometryEngine::decimateVoxelMesh(float ratio) {
    m_meshDecimation.decimate(m_voxelMesh, ratio);
}

// ── Export ───────────────────────────────────────────────────────────────────

std::string GeometryEngine::exportOBJ() const {
    std::vector<Mesh> allMeshes;

    // Add stroke meshes
    for (const auto& mesh : m_strokeMeshes) {
        if (!mesh.vertices.empty()) {
            allMeshes.push_back(mesh);
        }
    }

    // Add voxel mesh
    if (!m_voxelMesh.vertices.empty()) {
        allMeshes.push_back(m_voxelMesh);
    }

    if (allMeshes.empty()) return "";
    return m_meshExporter.exportOBJ(allMeshes);
}

std::vector<uint8_t> GeometryEngine::exportGLB() const {
    std::vector<Mesh> allMeshes;

    for (const auto& mesh : m_strokeMeshes) {
        if (!mesh.vertices.empty()) {
            allMeshes.push_back(mesh);
        }
    }

    if (!m_voxelMesh.vertices.empty()) {
        allMeshes.push_back(m_voxelMesh);
    }

    if (allMeshes.empty()) return {};
    return m_meshExporter.exportGLB(allMeshes);
}

bool GeometryEngine::exportOBJToFile(const std::string& path) const {
    Mesh combined = const_cast<GeometryEngine*>(this)->getCombinedStrokeMesh();
    if (!m_voxelMesh.vertices.empty()) {
        uint32_t offset = static_cast<uint32_t>(combined.vertices.size());
        combined.vertices.insert(combined.vertices.end(),
                                 m_voxelMesh.vertices.begin(), m_voxelMesh.vertices.end());
        for (uint32_t idx : m_voxelMesh.indices) {
            combined.indices.push_back(idx + offset);
        }
    }
    return m_meshExporter.exportOBJToFile(combined, path);
}

bool GeometryEngine::exportGLBToFile(const std::string& path) const {
    Mesh combined = const_cast<GeometryEngine*>(this)->getCombinedStrokeMesh();
    if (!m_voxelMesh.vertices.empty()) {
        uint32_t offset = static_cast<uint32_t>(combined.vertices.size());
        combined.vertices.insert(combined.vertices.end(),
                                 m_voxelMesh.vertices.begin(), m_voxelMesh.vertices.end());
        for (uint32_t idx : m_voxelMesh.indices) {
            combined.indices.push_back(idx + offset);
        }
    }
    return m_meshExporter.exportGLBToFile(combined, path);
}

// ── Scene Info ──────────────────────────────────────────────────────────────

int GeometryEngine::getTotalVertexCount() const {
    int count = 0;
    for (const auto& mesh : m_strokeMeshes) {
        count += static_cast<int>(mesh.vertices.size());
    }
    count += static_cast<int>(m_voxelMesh.vertices.size());
    return count;
}

int GeometryEngine::getTotalTriangleCount() const {
    int count = 0;
    for (const auto& mesh : m_strokeMeshes) {
        count += static_cast<int>(mesh.indices.size() / 3);
    }
    count += static_cast<int>(m_voxelMesh.indices.size() / 3);
    return count;
}

// ── Internal ────────────────────────────────────────────────────────────────

Mesh GeometryEngine::generateMeshFromStroke(const std::vector<StrokePoint>& points) {
    auto samples = m_splineGenerator.generate(points, 8);
    Mesh mesh = m_tubeMeshGenerator.generate(samples);
    mesh.computeNormals();
    return mesh;
}

} // namespace feather
