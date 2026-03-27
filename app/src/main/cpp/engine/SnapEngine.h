#pragma once

#include "../math/MathTypes.h"

namespace sky {

class SnapEngine {
public:
    /// Snap a point to a 3D grid of size `gridSize`
    static Vec3 snapToGrid(const Vec3& point, float gridSize);

    /// Snap a vector (from start to end) to the nearest angle increment
    /// Works best when drawing on primary planes (XY, XZ, YZ)
    static Vec3 snapToAngle(const Vec3& start, const Vec3& end, float snapDegrees);

    /// Constrain a point to move only along the given lockAxis (must be normalized)
    static Vec3 snapToAxis(const Vec3& start, const Vec3& end, const Vec3& lockAxis);

    /// Get straight line distance between a and b
    static float getLineLength(const Vec3& a, const Vec3& b);
};

} // namespace sky
