#include "TubeMeshGenerator.h"

namespace sky {

void TubeMeshGenerator::buildFrame(const Vec3& tangent, Vec3& outNormal, Vec3& outBinormal) const {
    // Choose a reference vector that isn't parallel to the tangent
    Vec3 ref = (std::abs(tangent.y) < 0.99f) ? Vec3(0.0f, 1.0f, 0.0f) : Vec3(1.0f, 0.0f, 0.0f);

    outBinormal = glm::normalize(glm::cross(tangent, ref));
    outNormal = glm::normalize(glm::cross(outBinormal, tangent));
}

Mesh TubeMeshGenerator::generate(const std::vector<SplineSample>& samples) const {
    Mesh mesh;

    if (samples.size() < 2) return mesh;

    int rings = static_cast<int>(samples.size());
    int segs = m_radialSegments;

    mesh.vertices.reserve(rings * segs);
    mesh.indices.reserve((rings - 1) * segs * 6);

    Vec3 prevNormal(0.0f), prevBinormal(0.0f);

    for (int i = 0; i < rings; ++i) {
        const SplineSample& s = samples[i];
        float radius = m_baseRadius * s.pressure;

        Vec3 normal, binormal;

        if (i == 0) {
            buildFrame(s.tangent, normal, binormal);
        } else {
            // Minimize twist: project previous frame onto current tangent plane
            Vec3 projected = prevNormal - glm::dot(prevNormal, s.tangent) * s.tangent;
            float projLen = glm::length(projected);

            if (projLen > 1e-6f) {
                normal = projected / projLen;
            } else {
                buildFrame(s.tangent, normal, binormal);
            }
            binormal = glm::normalize(glm::cross(s.tangent, normal));
        }

        prevNormal = normal;
        prevBinormal = binormal;

        // Generate circle vertices
        for (int j = 0; j < segs; ++j) {
            float angle = (2.0f * glm::pi<float>() * static_cast<float>(j)) / static_cast<float>(segs);
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);

            Vec3 circleDir = cosA * normal + sinA * binormal;

            Vertex vert;
            vert.position = s.position + circleDir * radius;
            vert.normal = circleDir;
            vert.uv = Vec2(
                static_cast<float>(j) / static_cast<float>(segs),
                s.t
            );
            vert.color = s.color;

            mesh.vertices.push_back(vert);
        }
    }

    // Connect rings with quads (2 triangles each)
    for (int i = 0; i < rings - 1; ++i) {
        for (int j = 0; j < segs; ++j) {
            int curr = i * segs + j;
            int next = i * segs + (j + 1) % segs;
            int currNext = (i + 1) * segs + j;
            int nextNext = (i + 1) * segs + (j + 1) % segs;

            // Triangle 1
            mesh.indices.push_back(curr);
            mesh.indices.push_back(currNext);
            mesh.indices.push_back(next);

            // Triangle 2
            mesh.indices.push_back(next);
            mesh.indices.push_back(currNext);
            mesh.indices.push_back(nextNext);
        }
    }

    // Cap the ends
    // Start cap
    {
        uint32_t centerIdx = static_cast<uint32_t>(mesh.vertices.size());
        Vertex center;
        center.position = samples.front().position;
        center.normal = -samples.front().tangent;
        center.uv = Vec2(0.5f, 0.0f);
        center.color = samples.front().color;
        mesh.vertices.push_back(center);

        for (int j = 0; j < segs; ++j) {
            int next = (j + 1) % segs;
            mesh.indices.push_back(centerIdx);
            mesh.indices.push_back(next);
            mesh.indices.push_back(j);
        }
    }

    // End cap
    {
        uint32_t centerIdx = static_cast<uint32_t>(mesh.vertices.size());
        Vertex center;
        center.position = samples.back().position;
        center.normal = samples.back().tangent;
        center.uv = Vec2(0.5f, 1.0f);
        center.color = samples.back().color;
        mesh.vertices.push_back(center);

        int lastRingStart = (rings - 1) * segs;
        for (int j = 0; j < segs; ++j) {
            int next = (j + 1) % segs;
            mesh.indices.push_back(centerIdx);
            mesh.indices.push_back(lastRingStart + j);
            mesh.indices.push_back(lastRingStart + next);
        }
    }

    return mesh;
}

} // namespace sky
