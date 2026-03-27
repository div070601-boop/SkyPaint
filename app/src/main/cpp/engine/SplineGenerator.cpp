#include "SplineGenerator.h"

namespace sky {

Vec3 SplineGenerator::catmullRom(
    const Vec3& p0, const Vec3& p1,
    const Vec3& p2, const Vec3& p3,
    float t, float tension
) {
    float t2 = t * t;
    float t3 = t2 * t;

    float s = (1.0f - tension) / 2.0f;

    Vec3 result =
        (2.0f * p1) +
        (-p0 + p2) * s * t +
        (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * s * t2 +
        (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * s * t3;

    // Correct Catmull-Rom formula
    float b0 = -s * t3 + 2.0f * s * t2 - s * t;
    float b1 = (2.0f - s) * t3 + (s - 3.0f) * t2 + 1.0f;
    float b2 = (s - 2.0f) * t3 + (3.0f - 2.0f * s) * t2 + s * t;
    float b3 = s * t3 - s * t2;

    return b0 * p0 + b1 * p1 + b2 * p2 + b3 * p3;
}

std::vector<SplineSample> SplineGenerator::generate(
    const std::vector<StrokePoint>& points,
    int samplesPerSegment
) const {
    std::vector<SplineSample> samples;

    if (points.size() < 2) return samples;

    int n = static_cast<int>(points.size());

    // Reserve approximate size
    samples.reserve((n - 1) * samplesPerSegment);

    for (int i = 0; i < n - 1; ++i) {
        // Get control points with clamped boundary
        int i0 = std::max(0, i - 1);
        int i1 = i;
        int i2 = std::min(n - 1, i + 1);
        int i3 = std::min(n - 1, i + 2);

        const Vec3& p0 = points[i0].position;
        const Vec3& p1 = points[i1].position;
        const Vec3& p2 = points[i2].position;
        const Vec3& p3 = points[i3].position;

        float press1 = points[i1].pressure;
        float press2 = points[i2].pressure;

        for (int j = 0; j < samplesPerSegment; ++j) {
            float t = static_cast<float>(j) / static_cast<float>(samplesPerSegment);

            SplineSample sample;
            sample.position = catmullRom(p0, p1, p2, p3, t, m_tension);

            // Tangent via finite difference
            float dt = 0.001f;
            Vec3 ahead = catmullRom(p0, p1, p2, p3, std::min(1.0f, t + dt), m_tension);
            sample.tangent = glm::normalize(ahead - sample.position);

            // Interpolate pressure
            sample.pressure = glm::mix(press1, press2, t);

            // Interpolate color
            sample.color = glm::mix(points[i1].color, points[i2].color, t);

            // Global parameter
            sample.t = (static_cast<float>(i) + t) / static_cast<float>(n - 1);

            samples.push_back(sample);
        }
    }

    // Add the last point
    SplineSample last;
    last.position = points.back().position;
    last.tangent = samples.empty() ? Vec3(0.0f, 1.0f, 0.0f) : samples.back().tangent;
    last.pressure = points.back().pressure;
    last.color = points.back().color;
    last.t = 1.0f;
    samples.push_back(last);

    return samples;
}

} // namespace sky
