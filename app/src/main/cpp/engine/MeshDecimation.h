#pragma once

#include "../math/MathTypes.h"

namespace sky {

/// Quadric error metric mesh decimation (edge collapse)
class MeshDecimation {
public:
    MeshDecimation() = default;

    /// Decimate mesh to target triangle count ratio (0.0 to 1.0)
    /// @param mesh   Input mesh (modified in-place)
    /// @param ratio  Target ratio (e.g., 0.5 = reduce to 50% of triangles)
    void decimate(Mesh& mesh, float ratio) const;

    /// Decimate to a specific target triangle count
    void decimateToCount(Mesh& mesh, int targetTriangles) const;

private:
    /// Quadric error matrix (symmetric 4x4 stored as 10 floats)
    struct Quadric {
        float data[10] = {0};

        Quadric() = default;

        void addPlane(const Vec3& normal, float d);
        float evaluate(const Vec3& v) const;
        Quadric operator+(const Quadric& other) const;
        Quadric& operator+=(const Quadric& other);

        /// Find optimal collapse point
        bool optimize(Vec3& result) const;
    };

    struct Edge {
        uint32_t v0, v1;
        float cost;
        Vec3 optimalPos;

        bool operator>(const Edge& other) const { return cost > other.cost; }
    };

    void computeQuadrics(const Mesh& mesh,
                         std::vector<Quadric>& quadrics) const;

    void compactMesh(Mesh& mesh) const;
};

} // namespace sky
