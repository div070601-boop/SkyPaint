#include "GeometryEngine.h"
#include "PrimitiveGenerator.h"
#include "RaycastEngine.h"
#include "SnapEngine.h"
#include <algorithm>

namespace sky {

GeometryEngine::GeometryEngine() {
    // Default tube parameters
    m_tubeMeshGenerator.setBaseRadius(0.005f);
    m_tubeMeshGenerator.setRadialSegments(12);
    m_splineGenerator.setTension(0.5f);

    // Default liquify parameters
    m_liquifyBrush.setRadius(0.05f);
    m_liquifyBrush.setStrength(0.5f);
}

// ── Undo / Redo ─────────────────────────────────────────────────────────────

void GeometryEngine::undo() {
    m_actionStack.undo();
    
    // Refresh meshes on undo out of band since stroke count might differ
    m_strokeMeshes.clear();
    for (int i = 0; i < m_strokeSystem.getStrokeCount(); ++i) {
        m_strokeMeshes.push_back(generateMeshFromStroke(m_strokeSystem.getStrokePoints(i)));
    }
}

void GeometryEngine::redo() {
    m_actionStack.redo();

    m_strokeMeshes.clear();
    for (int i = 0; i < m_strokeSystem.getStrokeCount(); ++i) {
        m_strokeMeshes.push_back(generateMeshFromStroke(m_strokeSystem.getStrokePoints(i)));
    }
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

        m_actionStack.pushAction(std::make_unique<StrokeAction>(m_strokeSystem, points));
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
    for (size_t i = 0; i < m_strokeMeshes.size(); ++i) {
        if (!isStrokeVisible(static_cast<int>(i))) continue;
        const auto& mesh = m_strokeMeshes[i];
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
    m_strokeMeshes.clear();
    m_strokeSystem.clearAll();
    m_actionStack.clear();
}

const std::vector<StrokePoint>& GeometryEngine::getStrokePoints(int strokeIndex) const {
    return m_strokeSystem.getStrokePoints(strokeIndex);
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

// ── Drafting State ──────────────────────────────────────────────────────────

void GeometryEngine::setStraightLineMode(bool enable) {
    m_strokeSystem.setStraightLineMode(enable);
}

void GeometryEngine::setGridSnap(bool enable, float size) {
    m_gridSnap = enable;
    m_gridSize = size;
    m_strokeSystem.setGridSnap(enable, size);
}

void GeometryEngine::setAngleSnap(bool enable, float degrees) {
    m_strokeSystem.setAngleSnap(enable, degrees);
}

float GeometryEngine::getCurrentStrokeLength() const {
    return m_strokeSystem.getCurrentStrokeLength();
}

// ── Voxel Sculpting ─────────────────────────────────────────────────────────

void GeometryEngine::initVoxelGrid(int resolution, const Vec3& boundsMin,
                                    const Vec3& boundsMax) {
    m_voxelGrid.initialize(resolution, boundsMin, boundsMax);
    m_voxelMeshDirty = true;
}

void GeometryEngine::beginSculpt() {
    if (m_voxelGrid.isInitialized()) {
        m_preSculptSnapshot = m_voxelGrid.getField();
    }
}

void GeometryEngine::sculptAt(const Vec3& position, float radius, float strength) {
    if (!m_voxelGrid.isInitialized()) return;

    if (m_drawMode == DrawMode::SCULPT_ADD) {
        m_voxelGrid.addSphere(position, radius, strength);
    } else if (m_drawMode == DrawMode::SCULPT_SUB) {
        m_voxelGrid.subtractSphere(position, radius, strength);
    } else if (m_drawMode == DrawMode::SCULPT_SMOOTH) {
        m_voxelGrid.smooth(position, radius);
    }

    m_voxelMeshDirty = true;
}

void GeometryEngine::endSculpt() {
    if (m_voxelGrid.isInitialized() && !m_preSculptSnapshot.empty()) {
        m_actionStack.pushAction(std::make_unique<SculptAction>(m_voxelGrid, m_preSculptSnapshot));
        m_preSculptSnapshot.clear();
    }
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

    // Add primitive meshes (transformed)
    for (const auto& obj : m_primitiveObjects) {
        if (!obj.mesh.vertices.empty()) {
            Mesh transformed = obj.mesh;
            for (auto& v : transformed.vertices) {
                Vec4 wp = obj.transform * Vec4(v.position, 1.0f);
                v.position = Vec3(wp.x, wp.y, wp.z);
                Vec4 wn = obj.transform * Vec4(v.normal, 0.0f);
                v.normal = glm::normalize(Vec3(wn.x, wn.y, wn.z));
            }
            allMeshes.push_back(transformed);
        }
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

    // Add primitive meshes (transformed)
    for (const auto& obj : m_primitiveObjects) {
        if (!obj.mesh.vertices.empty()) {
            Mesh transformed = obj.mesh;
            for (auto& v : transformed.vertices) {
                Vec4 wp = obj.transform * Vec4(v.position, 1.0f);
                v.position = Vec3(wp.x, wp.y, wp.z);
                Vec4 wn = obj.transform * Vec4(v.normal, 0.0f);
                v.normal = glm::normalize(Vec3(wn.x, wn.y, wn.z));
            }
            allMeshes.push_back(transformed);
        }
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
    // Add primitive meshes (transformed)
    for (const auto& obj : m_primitiveObjects) {
        if (!obj.mesh.vertices.empty()) {
            uint32_t offset = static_cast<uint32_t>(combined.vertices.size());
            for (const auto& v : obj.mesh.vertices) {
                Vertex tv = v;
                Vec4 wp = obj.transform * Vec4(v.position, 1.0f);
                tv.position = Vec3(wp.x, wp.y, wp.z);
                Vec4 wn = obj.transform * Vec4(v.normal, 0.0f);
                tv.normal = glm::normalize(Vec3(wn.x, wn.y, wn.z));
                combined.vertices.push_back(tv);
            }
            for (uint32_t idx : obj.mesh.indices) {
                combined.indices.push_back(idx + offset);
            }
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
    // Add primitive meshes (transformed)
    for (const auto& obj : m_primitiveObjects) {
        if (!obj.mesh.vertices.empty()) {
            uint32_t offset = static_cast<uint32_t>(combined.vertices.size());
            for (const auto& v : obj.mesh.vertices) {
                Vertex tv = v;
                Vec4 wp = obj.transform * Vec4(v.position, 1.0f);
                tv.position = Vec3(wp.x, wp.y, wp.z);
                Vec4 wn = obj.transform * Vec4(v.normal, 0.0f);
                tv.normal = glm::normalize(Vec3(wn.x, wn.y, wn.z));
                combined.vertices.push_back(tv);
            }
            for (uint32_t idx : obj.mesh.indices) {
                combined.indices.push_back(idx + offset);
            }
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
    for (const auto& obj : m_primitiveObjects) {
        count += static_cast<int>(obj.mesh.vertices.size());
    }
    return count;
}

int GeometryEngine::getTotalTriangleCount() const {
    int count = 0;
    for (const auto& mesh : m_strokeMeshes) {
        count += static_cast<int>(mesh.indices.size() / 3);
    }
    count += static_cast<int>(m_voxelMesh.indices.size() / 3);
    for (const auto& obj : m_primitiveObjects) {
        count += static_cast<int>(obj.mesh.indices.size() / 3);
    }
    return count;
}

// ── Internal ────────────────────────────────────────────────────────────────

Mesh GeometryEngine::generateMeshFromStroke(const std::vector<StrokePoint>& points) {
    auto samples = m_splineGenerator.generate(points, 8);
    Mesh mesh = m_tubeMeshGenerator.generate(samples);
    mesh.computeNormals();
    return mesh;
}


// ── Primitive Objects ───────────────────────────────────────────────────────

int GeometryEngine::addPrimitive(PrimitiveType type, const Mat4& transform, const Vec4& color) {
    SceneObject obj;
    obj.id = m_nextObjectId++;
    obj.transform = transform;
    obj.color = color;
    obj.type = ObjectType::PRIMITIVE;
    obj.primitiveType = static_cast<int>(type);

    switch (type) {
        case PrimitiveType::CUBE:     obj.mesh = PrimitiveGenerator::generateCube(); break;
        case PrimitiveType::SPHERE:   obj.mesh = PrimitiveGenerator::generateSphere(); break;
        case PrimitiveType::CYLINDER: obj.mesh = PrimitiveGenerator::generateCylinder(); break;
        case PrimitiveType::CONE:     obj.mesh = PrimitiveGenerator::generateCone(); break;
        case PrimitiveType::PLANE:    obj.mesh = PrimitiveGenerator::generatePlane(); break;
        case PrimitiveType::TORUS:    obj.mesh = PrimitiveGenerator::generateTorus(); break;
    }

    if (m_gridSnap && m_gridSize > 0.001f) {
        Vec3 pos(obj.transform[12], obj.transform[13], obj.transform[14]);
        pos = SnapEngine::snapToGrid(pos, m_gridSize);
        obj.transform[12] = pos.x;
        obj.transform[13] = pos.y;
        obj.transform[14] = pos.z;
    }

    m_primitiveObjects.push_back(obj);

    // Push undo action
    m_actionStack.pushAction(std::make_unique<PrimitiveAddAction>(m_primitiveObjects, obj));

    return obj.id;
}

const SceneObject* GeometryEngine::getPrimitive(int index) const {
    if (index >= 0 && index < static_cast<int>(m_primitiveObjects.size())) {
        return &m_primitiveObjects[index];
    }
    return nullptr;
}

int GeometryEngine::getPrimitiveCount() const {
    return static_cast<int>(m_primitiveObjects.size());
}

void GeometryEngine::removePrimitive(int index) {
    if (index >= 0 && index < static_cast<int>(m_primitiveObjects.size())) {
        m_primitiveObjects.erase(m_primitiveObjects.begin() + index);
    }
}

void GeometryEngine::clearPrimitives() {
    m_primitiveObjects.clear();
}

// ── Object Properties ───────────────────────────────────────────────────────

bool GeometryEngine::isPrimitiveVisible(int index) const {
    if (index >= 0 && index < static_cast<int>(m_primitiveObjects.size())) {
        return m_primitiveObjects[index].visible;
    }
    return false;
}

void GeometryEngine::setPrimitiveVisible(int index, bool visible) {
    if (index >= 0 && index < static_cast<int>(m_primitiveObjects.size())) {
        m_primitiveObjects[index].visible = visible;
    }
}

bool GeometryEngine::isPrimitiveLocked(int index) const {
    if (index >= 0 && index < static_cast<int>(m_primitiveObjects.size())) {
        return m_primitiveObjects[index].locked;
    }
    return false;
}

void GeometryEngine::setPrimitiveLocked(int index, bool locked) {
    if (index >= 0 && index < static_cast<int>(m_primitiveObjects.size())) {
        m_primitiveObjects[index].locked = locked;
    }
}

std::string GeometryEngine::getPrimitiveName(int index) const {
    if (index >= 0 && index < static_cast<int>(m_primitiveObjects.size())) {
        return m_primitiveObjects[index].name;
    }
    return "";
}

void GeometryEngine::setPrimitiveName(int index, const std::string& name) {
    if (index >= 0 && index < static_cast<int>(m_primitiveObjects.size())) {
        m_primitiveObjects[index].name = name;
    }
}

// Strokes
bool GeometryEngine::isStrokeVisible(int index) const {
    return m_strokeSystem.isVisible(index);
}

void GeometryEngine::setStrokeVisible(int index, bool visible) {
    m_strokeSystem.setVisible(index, visible);
    // Since strokes are exported as one big mesh, we might need a way to filter them during build,
    // but for now, we just update the flag.
}

bool GeometryEngine::isStrokeLocked(int index) const {
    return m_strokeSystem.isLocked(index);
}

void GeometryEngine::setStrokeLocked(int index, bool locked) {
    m_strokeSystem.setLocked(index, locked);
}

std::string GeometryEngine::getStrokeName(int index) const {
    return m_strokeSystem.getName(index);
}

void GeometryEngine::setStrokeName(int index, const std::string& name) {
    m_strokeSystem.setName(index, name);
}

// ── Selection & Transform ───────────────────────────────────────────────────

int GeometryEngine::pickObjectAt(float rayOx, float rayOy, float rayOz,
                                  float rayDx, float rayDy, float rayDz) {
    Ray ray;
    ray.origin = Vec3(rayOx, rayOy, rayOz);
    ray.direction = glm::normalize(Vec3(rayDx, rayDy, rayDz));

    // First try primitives (ignore hidden or locked)
    RayHit hit = RaycastEngine::pickObject(ray, m_primitiveObjects, [this](int idx) {
        if (idx < 0 || idx >= static_cast<int>(m_primitiveObjects.size())) return false;
        return m_primitiveObjects[idx].visible && !m_primitiveObjects[idx].locked;
    });
    
    if (hit.objectId >= 0) {
        m_selectedObjectId = hit.objectId;
        return hit.objectId;
    }

    // Then try strokes
    int strokeIdx = RaycastEngine::pickStroke(ray, m_strokeMeshes, [this](int idx) {
        return isStrokeVisible(idx) && !isStrokeLocked(idx);
    });
    
    if (strokeIdx >= 0) {
        // Use negative IDs for strokes: -(strokeIndex + 1)
        m_selectedObjectId = -(strokeIdx + 1);
        return m_selectedObjectId;
    }

    m_selectedObjectId = -1;
    return -1;
}

void GeometryEngine::transformPrimitive(int index, const Mat4& transform) {
    if (index >= 0 && index < static_cast<int>(m_primitiveObjects.size())) {
        Mat4 newTransform = transform;
        if (m_gridSnap && m_gridSize > 0.001f) {
            Vec3 pos(newTransform[12], newTransform[13], newTransform[14]);
            pos = SnapEngine::snapToGrid(pos, m_gridSize);
            newTransform[12] = pos.x;
            newTransform[13] = pos.y;
            newTransform[14] = pos.z;
        }

        Mat4 oldTransform = m_primitiveObjects[index].transform;
        m_primitiveObjects[index].transform = newTransform;
        m_actionStack.pushAction(std::make_unique<PrimitiveTransformAction>(
            m_primitiveObjects, index, oldTransform, newTransform));
    }
}

void GeometryEngine::mergeSelectedPrimitive(bool subtract) {
    if (m_selectedObjectId >= 0 && m_selectedObjectId < static_cast<int>(m_primitiveObjects.size())) {
        // Capture old state for undo
        std::vector<float> oldFieldSnapshot = m_voxelGrid.getField();
        int primitiveIndex = m_selectedObjectId;

        const auto& obj = m_primitiveObjects[primitiveIndex];
        m_voxelGrid.mergePrimitiveSDF(obj.primitiveType, obj.transform, subtract);
        m_voxelMeshDirty = true;
        
        // Hide it
        m_primitiveObjects[primitiveIndex].visible = false;
        m_selectedObjectId = -1;

        // Push to undo stack
        m_actionStack.pushAction(std::make_unique<PrimitiveCSGAction>(
            m_voxelGrid, m_primitiveObjects, primitiveIndex, oldFieldSnapshot));
    }
}

} // namespace sky
