#include "RaycastEngine.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace feather {

float RaycastEngine::rayAABB(const Ray& ray, const Vec3& bmin, const Vec3& bmax) {
    float tmin = -std::numeric_limits<float>::infinity();
    float tmax =  std::numeric_limits<float>::infinity();

    for (int i = 0; i < 3; ++i) {
        float invD = 1.0f / ray.direction[i];
        float t0 = (bmin[i] - ray.origin[i]) * invD;
        float t1 = (bmax[i] - ray.origin[i]) * invD;

        if (invD < 0.0f) std::swap(t0, t1);

        tmin = std::max(tmin, t0);
        tmax = std::min(tmax, t1);

        if (tmax < tmin) return -1.0f;
    }

    if (tmin < 0.0f) {
        // Camera is inside the box; use tmax
        if (tmax < 0.0f) return -1.0f;
        return tmax;
    }
    return tmin;
}

void RaycastEngine::computeAABB(const Mesh& mesh, Vec3& outMin, Vec3& outMax) {
    if (mesh.vertices.empty()) {
        outMin = Vec3(0.0f);
        outMax = Vec3(0.0f);
        return;
    }

    outMin = Vec3( std::numeric_limits<float>::max());
    outMax = Vec3(-std::numeric_limits<float>::max());

    for (const auto& v : mesh.vertices) {
        outMin = glm::min(outMin, v.position);
        outMax = glm::max(outMax, v.position);
    }
}

RayHit RaycastEngine::pickObject(const Ray& ray, const std::vector<SceneObject>& objects) {
    RayHit bestHit;

    for (const auto& obj : objects) {
        // Compute local AABB
        Vec3 localMin, localMax;
        computeAABB(obj.mesh, localMin, localMax);

        // Transform AABB corners by the object's transform to get world AABB
        // For simplicity, we transform all 8 corners and recompute AABB
        Vec3 worldMin( std::numeric_limits<float>::max());
        Vec3 worldMax(-std::numeric_limits<float>::max());

        for (int c = 0; c < 8; ++c) {
            Vec3 corner(
                (c & 1) ? localMax.x : localMin.x,
                (c & 2) ? localMax.y : localMin.y,
                (c & 4) ? localMax.z : localMin.z
            );
            Vec4 worldCorner = obj.transform * Vec4(corner, 1.0f);
            Vec3 wc(worldCorner.x, worldCorner.y, worldCorner.z);
            worldMin = glm::min(worldMin, wc);
            worldMax = glm::max(worldMax, wc);
        }

        float t = rayAABB(ray, worldMin, worldMax);
        if (t >= 0.0f && t < bestHit.distance) {
            bestHit.objectId = obj.id;
            bestHit.distance = t;
            bestHit.hitPoint = ray.origin + ray.direction * t;
        }
    }

    return bestHit;
}

int RaycastEngine::pickStroke(const Ray& ray, const std::vector<Mesh>& strokeMeshes) {
    float bestDist = std::numeric_limits<float>::max();
    int bestIndex = -1;

    for (int i = 0; i < static_cast<int>(strokeMeshes.size()); ++i) {
        Vec3 bmin, bmax;
        computeAABB(strokeMeshes[i], bmin, bmax);

        // Expand AABB slightly for easier selection
        Vec3 expand(0.02f);
        bmin -= expand;
        bmax += expand;

        float t = rayAABB(ray, bmin, bmax);
        if (t >= 0.0f && t < bestDist) {
            bestDist = t;
            bestIndex = i;
        }
    }

    return bestIndex;
}

} // namespace feather
