#pragma once

#include "../math/MathTypes.h"

namespace feather {

/// Vertex deformation brush with smooth falloff
class LiquifyBrush {
public:
    LiquifyBrush() = default;

    /// Set brush radius in world units
    void setRadius(float radius) { m_radius = radius; }
    float getRadius() const { return m_radius; }

    /// Set brush strength (0.0 to 1.0)
    void setStrength(float strength) { m_strength = strength; }
    float getStrength() const { return m_strength; }

    /// Apply deformation to a mesh
    /// @param mesh          The mesh to deform (modified in-place)
    /// @param brushCenter   World-space center of the brush
    /// @param direction     Direction to push vertices
    void apply(Mesh& mesh, const Vec3& brushCenter, const Vec3& direction) const;

    /// Smooth vertices around brush center (relaxation)
    void smooth(Mesh& mesh, const Vec3& brushCenter, int iterations = 1) const;

    /// Inflate/deflate along normals
    void inflate(Mesh& mesh, const Vec3& brushCenter, float amount) const;

    /// Pinch vertices toward brush center
    void pinch(Mesh& mesh, const Vec3& brushCenter) const;

private:
    float m_radius = 0.05f;
    float m_strength = 0.5f;

    /// Smooth falloff function (hermite)
    float falloff(float distance) const;

    /// Find vertices within the brush radius
    std::vector<uint32_t> findAffectedVertices(const Mesh& mesh,
                                                const Vec3& center) const;
};

} // namespace feather
