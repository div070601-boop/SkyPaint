#include "LiquifyBrush.h"

namespace feather {

float LiquifyBrush::falloff(float distance) const {
    if (distance >= m_radius) return 0.0f;
    float t = distance / m_radius;
    // Hermite smooth falloff: 1 at center, 0 at edge
    return (1.0f - t) * (1.0f - t) * (1.0f + 2.0f * t);
}

std::vector<uint32_t> LiquifyBrush::findAffectedVertices(
    const Mesh& mesh, const Vec3& center) const {
    std::vector<uint32_t> result;
    result.reserve(128);

    for (uint32_t i = 0; i < static_cast<uint32_t>(mesh.vertices.size()); ++i) {
        float dist = glm::distance(mesh.vertices[i].position, center);
        if (dist < m_radius) {
            result.push_back(i);
        }
    }
    return result;
}

void LiquifyBrush::apply(Mesh& mesh, const Vec3& brushCenter,
                          const Vec3& direction) const {
    auto affected = findAffectedVertices(mesh, brushCenter);

    Vec3 normalizedDir = glm::length(direction) > 1e-6f
        ? glm::normalize(direction) : Vec3(0.0f);

    for (uint32_t idx : affected) {
        float dist = glm::distance(mesh.vertices[idx].position, brushCenter);
        float weight = falloff(dist) * m_strength;
        mesh.vertices[idx].position += normalizedDir * weight * m_radius * 0.1f;
    }

    mesh.dirty = true;
}

void LiquifyBrush::smooth(Mesh& mesh, const Vec3& brushCenter,
                           int iterations) const {
    auto affected = findAffectedVertices(mesh, brushCenter);
    if (affected.empty()) return;

    // Build adjacency for affected vertices
    // Simple approach: for each affected vertex, average with neighbors in triangles
    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<Vec3> newPositions(mesh.vertices.size());
        std::vector<int> counts(mesh.vertices.size(), 0);

        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            newPositions[i] = mesh.vertices[i].position;
        }

        // Accumulate neighbor positions from triangle adjacency
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            uint32_t i0 = mesh.indices[i];
            uint32_t i1 = mesh.indices[i + 1];
            uint32_t i2 = mesh.indices[i + 2];

            newPositions[i0] += mesh.vertices[i1].position + mesh.vertices[i2].position;
            newPositions[i1] += mesh.vertices[i0].position + mesh.vertices[i2].position;
            newPositions[i2] += mesh.vertices[i0].position + mesh.vertices[i1].position;
            counts[i0] += 2;
            counts[i1] += 2;
            counts[i2] += 2;
        }

        // Apply smoothing only to affected vertices
        for (uint32_t idx : affected) {
            if (counts[idx] > 0) {
                Vec3 smoothed = newPositions[idx] / static_cast<float>(counts[idx] + 1);
                float dist = glm::distance(mesh.vertices[idx].position, brushCenter);
                float weight = falloff(dist) * m_strength;
                mesh.vertices[idx].position = glm::mix(
                    mesh.vertices[idx].position, smoothed, weight);
            }
        }
    }

    mesh.dirty = true;
}

void LiquifyBrush::inflate(Mesh& mesh, const Vec3& brushCenter,
                            float amount) const {
    auto affected = findAffectedVertices(mesh, brushCenter);

    for (uint32_t idx : affected) {
        float dist = glm::distance(mesh.vertices[idx].position, brushCenter);
        float weight = falloff(dist) * m_strength;
        Vec3& normal = mesh.vertices[idx].normal;

        if (glm::length(normal) > 1e-6f) {
            mesh.vertices[idx].position += glm::normalize(normal) * amount * weight;
        }
    }

    mesh.dirty = true;
}

void LiquifyBrush::pinch(Mesh& mesh, const Vec3& brushCenter) const {
    auto affected = findAffectedVertices(mesh, brushCenter);

    for (uint32_t idx : affected) {
        float dist = glm::distance(mesh.vertices[idx].position, brushCenter);
        float weight = falloff(dist) * m_strength;

        Vec3 toCenter = brushCenter - mesh.vertices[idx].position;
        mesh.vertices[idx].position += toCenter * weight * 0.1f;
    }

    mesh.dirty = true;
}

} // namespace feather
