#include "MeshDecimation.h"
#include <queue>
#include <set>
#include <unordered_set>
#include <functional>

namespace sky {

// ── Quadric Implementation ──────────────────────────────────────────────────

void MeshDecimation::Quadric::addPlane(const Vec3& n, float d) {
    // Quadric from plane equation: Q = n*n^T extended with d
    // Stored as upper triangular: a² ab ac ad  b² bc bd  c² cd  d²
    data[0] += n.x * n.x;
    data[1] += n.x * n.y;
    data[2] += n.x * n.z;
    data[3] += n.x * d;
    data[4] += n.y * n.y;
    data[5] += n.y * n.z;
    data[6] += n.y * d;
    data[7] += n.z * n.z;
    data[8] += n.z * d;
    data[9] += d * d;
}

float MeshDecimation::Quadric::evaluate(const Vec3& v) const {
    // v^T * Q * v
    return data[0] * v.x * v.x + 2.0f * data[1] * v.x * v.y +
           2.0f * data[2] * v.x * v.z + 2.0f * data[3] * v.x +
           data[4] * v.y * v.y + 2.0f * data[5] * v.y * v.z +
           2.0f * data[6] * v.y + data[7] * v.z * v.z +
           2.0f * data[8] * v.z + data[9];
}

MeshDecimation::Quadric MeshDecimation::Quadric::operator+(const Quadric& other) const {
    Quadric result;
    for (int i = 0; i < 10; ++i) {
        result.data[i] = data[i] + other.data[i];
    }
    return result;
}

MeshDecimation::Quadric& MeshDecimation::Quadric::operator+=(const Quadric& other) {
    for (int i = 0; i < 10; ++i) {
        data[i] += other.data[i];
    }
    return *this;
}

bool MeshDecimation::Quadric::optimize(Vec3& result) const {
    // Solve the 3x3 linear system from dQ/dv = 0
    // [ a00 a01 a02 ] [x]   [-a03]
    // [ a01 a04 a05 ] [y] = [-a06]
    // [ a02 a05 a07 ] [z]   [-a08]
    float a00 = data[0], a01 = data[1], a02 = data[2], a03 = data[3];
    float a04 = data[4], a05 = data[5], a06 = data[6];
    float a07 = data[7], a08 = data[8];

    float det = a00 * (a04 * a07 - a05 * a05)
              - a01 * (a01 * a07 - a05 * a02)
              + a02 * (a01 * a05 - a04 * a02);

    if (std::abs(det) < 1e-10f) return false;

    float invDet = 1.0f / det;

    result.x = invDet * (-(a04 * a07 - a05 * a05) * a03
                         + (a01 * a07 - a05 * a02) * a06
                         - (a01 * a05 - a04 * a02) * a08);

    result.y = invDet * ((a01 * a07 - a02 * a05) * a03
                         - (a00 * a07 - a02 * a02) * a06
                         + (a00 * a05 - a01 * a02) * a08);

    result.z = invDet * (-(a01 * a05 - a02 * a04) * a03
                         + (a00 * a05 - a01 * a02) * a06
                         - (a00 * a04 - a01 * a01) * a08);

    return true;
}

// ── Decimation Implementation ───────────────────────────────────────────────

void MeshDecimation::computeQuadrics(const Mesh& mesh,
                                      std::vector<Quadric>& quadrics) const {
    quadrics.resize(mesh.vertices.size());

    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        uint32_t i0 = mesh.indices[i];
        uint32_t i1 = mesh.indices[i + 1];
        uint32_t i2 = mesh.indices[i + 2];

        const Vec3& p0 = mesh.vertices[i0].position;
        const Vec3& p1 = mesh.vertices[i1].position;
        const Vec3& p2 = mesh.vertices[i2].position;

        Vec3 e1 = p1 - p0;
        Vec3 e2 = p2 - p0;
        Vec3 normal = glm::cross(e1, e2);
        float area = glm::length(normal);

        if (area < 1e-8f) continue;
        normal /= area;

        float d = -glm::dot(normal, p0);

        Quadric q;
        q.addPlane(normal, d);

        quadrics[i0] += q;
        quadrics[i1] += q;
        quadrics[i2] += q;
    }
}

void MeshDecimation::decimate(Mesh& mesh, float ratio) const {
    int targetTris = static_cast<int>(
        static_cast<float>(mesh.indices.size() / 3) * ratio);
    decimateToCount(mesh, targetTris);
}

void MeshDecimation::decimateToCount(Mesh& mesh, int targetTriangles) const {
    if (mesh.indices.size() / 3 <= static_cast<size_t>(targetTriangles)) return;

    // Compute initial quadrics
    std::vector<Quadric> quadrics;
    computeQuadrics(mesh, quadrics);

    // Build edge set
    std::set<std::pair<uint32_t, uint32_t>> edgeSet;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        uint32_t idx[3] = {mesh.indices[i], mesh.indices[i+1], mesh.indices[i+2]};
        for (int j = 0; j < 3; ++j) {
            uint32_t a = idx[j];
            uint32_t b = idx[(j + 1) % 3];
            if (a > b) std::swap(a, b);
            edgeSet.insert({a, b});
        }
    }

    // Priority queue of edges sorted by cost
    auto edgeCmp = [](const Edge& a, const Edge& b) { return a.cost > b.cost; };
    std::priority_queue<Edge, std::vector<Edge>, decltype(edgeCmp)> pq(edgeCmp);

    // Track which vertices have been collapsed
    std::vector<uint32_t> vertexMap(mesh.vertices.size());
    for (uint32_t i = 0; i < vertexMap.size(); ++i) vertexMap[i] = i;

    auto findRoot = [&](uint32_t v) -> uint32_t {
        while (vertexMap[v] != v) {
            vertexMap[v] = vertexMap[vertexMap[v]]; // path compression
            v = vertexMap[v];
        }
        return v;
    };

    // Compute edge costs and add to queue
    for (const auto& e : edgeSet) {
        Edge edge;
        edge.v0 = e.first;
        edge.v1 = e.second;

        Quadric combined = quadrics[edge.v0] + quadrics[edge.v1];

        if (!combined.optimize(edge.optimalPos)) {
            // Fallback: midpoint
            edge.optimalPos = (mesh.vertices[edge.v0].position +
                              mesh.vertices[edge.v1].position) * 0.5f;
        }

        edge.cost = std::max(0.0f, combined.evaluate(edge.optimalPos));
        pq.push(edge);
    }

    int currentTris = static_cast<int>(mesh.indices.size() / 3);
    std::unordered_set<uint32_t> removedVertices;

    while (currentTris > targetTriangles && !pq.empty()) {
        Edge edge = pq.top();
        pq.pop();

        uint32_t v0 = findRoot(edge.v0);
        uint32_t v1 = findRoot(edge.v1);

        if (v0 == v1) continue; // already collapsed
        if (removedVertices.count(v0) || removedVertices.count(v1)) continue;

        // Collapse v1 into v0
        mesh.vertices[v0].position = edge.optimalPos;
        mesh.vertices[v0].normal = glm::normalize(
            mesh.vertices[v0].normal + mesh.vertices[v1].normal);
        vertexMap[v1] = v0;
        quadrics[v0] += quadrics[v1];
        removedVertices.insert(v1);

        // Update triangles and count removals
        int removed = 0;
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            for (int j = 0; j < 3; ++j) {
                mesh.indices[i + j] = findRoot(mesh.indices[i + j]);
            }

            // Check for degenerate triangles
            if (mesh.indices[i] == mesh.indices[i + 1] ||
                mesh.indices[i] == mesh.indices[i + 2] ||
                mesh.indices[i + 1] == mesh.indices[i + 2]) {
                mesh.indices[i] = mesh.indices[i + 1] = mesh.indices[i + 2] = UINT32_MAX;
                removed++;
            }
        }
        currentTris -= removed;
    }

    // Remove degenerate triangles
    std::vector<uint32_t> newIndices;
    newIndices.reserve(mesh.indices.size());
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        if (mesh.indices[i] != UINT32_MAX) {
            newIndices.push_back(mesh.indices[i]);
            newIndices.push_back(mesh.indices[i + 1]);
            newIndices.push_back(mesh.indices[i + 2]);
        }
    }
    mesh.indices = std::move(newIndices);

    compactMesh(mesh);
    mesh.computeNormals();
    mesh.dirty = true;
}

void MeshDecimation::compactMesh(Mesh& mesh) const {
    // Find which vertices are actually referenced
    std::vector<bool> used(mesh.vertices.size(), false);
    for (uint32_t idx : mesh.indices) {
        if (idx < used.size()) used[idx] = true;
    }

    // Build remap
    std::vector<uint32_t> remap(mesh.vertices.size(), UINT32_MAX);
    std::vector<Vertex> compacted;
    compacted.reserve(mesh.vertices.size());

    for (uint32_t i = 0; i < mesh.vertices.size(); ++i) {
        if (used[i]) {
            remap[i] = static_cast<uint32_t>(compacted.size());
            compacted.push_back(mesh.vertices[i]);
        }
    }

    // Remap indices
    for (auto& idx : mesh.indices) {
        idx = remap[idx];
    }

    mesh.vertices = std::move(compacted);
}

} // namespace sky
