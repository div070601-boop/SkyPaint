#include "SnapEngine.h"
#include <cmath>

namespace sky {

Vec3 SnapEngine::snapToGrid(const Vec3& point, float gridSize) {
    if (gridSize <= 0.0f) return point;
    return Vec3(
        std::round(point.x / gridSize) * gridSize,
        std::round(point.y / gridSize) * gridSize,
        std::round(point.z / gridSize) * gridSize
    );
}

Vec3 SnapEngine::snapToAngle(const Vec3& start, const Vec3& end, float snapDegrees) {
    if (snapDegrees <= 0.0f) return end;

    Vec3 dir = end - start;
    float len = glm::length(dir);
    if (len < 1e-6f) return end;
    dir /= len;

    // Search for the closest snapped vector on the 3 primary planes (XY, XZ, YZ)
    float maxDot = -2.0f;
    Vec3 bestAxis = dir;

    int steps = static_cast<int>(360.0f / snapDegrees);
    float radStep = glm::radians(snapDegrees);

    for (int plane = 0; plane < 3; ++plane) {
        for (int i = 0; i < steps; ++i) {
            float angle = static_cast<float>(i) * radStep;
            Vec3 testAxis(0.0f);
            
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);

            if (plane == 0)      testAxis = Vec3(cosA, sinA, 0.0f); // XY
            else if (plane == 1) testAxis = Vec3(cosA, 0.0f, sinA); // XZ
            else                 testAxis = Vec3(0.0f, cosA, sinA); // YZ

            float d = glm::dot(dir, testAxis);
            if (d > maxDot) {
                maxDot = d;
                bestAxis = testAxis;
            }
        }
    }

    return start + bestAxis * len;
}

Vec3 SnapEngine::snapToAxis(const Vec3& start, const Vec3& end, const Vec3& lockAxis) {
    Vec3 dir = end - start;
    float projectionLength = glm::dot(dir, glm::normalize(lockAxis));
    return start + glm::normalize(lockAxis) * projectionLength;
}

float SnapEngine::getLineLength(const Vec3& a, const Vec3& b) {
    return glm::distance(a, b);
}

} // namespace sky
