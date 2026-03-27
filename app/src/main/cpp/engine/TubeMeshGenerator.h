#pragma once

#include "../math/MathTypes.h"
#include "SplineGenerator.h"

namespace sky {

/// Generates tube geometry by sweeping a circle along a spline
class TubeMeshGenerator {
public:
    TubeMeshGenerator() = default;

    /// Set the number of radial segments (circle resolution)
    void setRadialSegments(int segments) { m_radialSegments = segments; }

    /// Set base radius of the tube
    void setBaseRadius(float radius) { m_baseRadius = radius; }

    /// Generate tube mesh from spline samples
    Mesh generate(const std::vector<SplineSample>& samples) const;

private:
    int m_radialSegments = 12;
    float m_baseRadius = 0.005f; // 5mm default

    /// Build a coordinate frame from a tangent vector
    void buildFrame(const Vec3& tangent, Vec3& outNormal, Vec3& outBinormal) const;
};

} // namespace sky
