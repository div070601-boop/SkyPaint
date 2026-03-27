#pragma once

#include "../math/MathTypes.h"
#include "SceneObject.h"
#include <vector>
#include <functional>

namespace feather {

struct Ray {
    Vec3 origin;
    Vec3 direction;
};

struct RayHit {
    int objectId = -1;
    float distance = 1e30f;
    Vec3 hitPoint{0.0f};
};

class RaycastEngine {
public:
    /**
     * Test a ray against an axis-aligned bounding box (AABB).
     * Returns distance along ray, or -1 if no hit.
     */
    static float rayAABB(const Ray& ray, const Vec3& bmin, const Vec3& bmax);

    /**
     * Compute the AABB of a mesh (in local space).
     */
    static void computeAABB(const Mesh& mesh, Vec3& outMin, Vec3& outMax);

    /**
     * Pick the closest SceneObject hit by a ray.
     * Transforms each object's AABB by its transform matrix before testing.
     * Returns the hit info (objectId == -1 if nothing hit).
     */
    static RayHit pickObject(const Ray& ray, const std::vector<SceneObject>& objects, std::function<bool(int)> filter = nullptr);

    static int pickStroke(const Ray& ray, const std::vector<Mesh>& strokeMeshes, std::function<bool(int)> filter = nullptr);
};

} // namespace feather
