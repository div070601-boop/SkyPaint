#pragma once

#include "../math/MathTypes.h"

namespace feather {

/// Catmull-Rom spline evaluated from stroke points
struct SplineSample {
    Vec3 position;
    Vec3 tangent;
    float pressure;
    float t; // parameter [0, 1]
};

class SplineGenerator {
public:
    SplineGenerator() = default;

    /// Set spline tension (0.0 to 1.0, default 0.5 = standard Catmull-Rom)
    void setTension(float tension) { m_tension = tension; }

    /// Generate spline samples from stroke points
    /// samplesPerSegment: how many samples between each pair of control points
    std::vector<SplineSample> generate(
        const std::vector<StrokePoint>& points,
        int samplesPerSegment = 8
    ) const;

    /// Evaluate a single point on the Catmull-Rom spline
    static Vec3 catmullRom(
        const Vec3& p0, const Vec3& p1,
        const Vec3& p2, const Vec3& p3,
        float t, float tension = 0.5f
    );

private:
    float m_tension = 0.5f;
};

} // namespace feather
