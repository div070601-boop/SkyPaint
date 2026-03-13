#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <array>
#include <unordered_map>
#include <memory>

namespace feather {

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;
using Quat = glm::quat;

struct StrokePoint {
    Vec3 position{0.0f};
    float pressure = 1.0f;
    float tilt = 0.0f;
    float timestamp = 0.0f;
};

struct Vertex {
    Vec3 position{0.0f};
    Vec3 normal{0.0f};
    Vec2 uv{0.0f};
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    bool dirty = true;

    void clear() {
        vertices.clear();
        indices.clear();
        dirty = true;
    }

    void computeNormals() {
        // Reset normals
        for (auto& v : vertices) {
            v.normal = Vec3(0.0f);
        }

        // Accumulate face normals
        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];

            Vec3 e1 = vertices[i1].position - vertices[i0].position;
            Vec3 e2 = vertices[i2].position - vertices[i0].position;
            Vec3 n = glm::cross(e1, e2);

            vertices[i0].normal += n;
            vertices[i1].normal += n;
            vertices[i2].normal += n;
        }

        // Normalize
        for (auto& v : vertices) {
            float len = glm::length(v.normal);
            if (len > 1e-6f) {
                v.normal /= len;
            }
        }
    }
};

struct BoundingBox {
    Vec3 min{std::numeric_limits<float>::max()};
    Vec3 max{std::numeric_limits<float>::lowest()};

    void expand(const Vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    Vec3 center() const { return (min + max) * 0.5f; }
    Vec3 size() const { return max - min; }
};

} // namespace feather
