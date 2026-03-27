#pragma once

#include "../math/MathTypes.h"
#include "VoxelGrid.h"

namespace sky {

/// Classic Marching Cubes implementation
class MarchingCubes {
public:
    MarchingCubes() = default;

    /// Set the iso-level for surface extraction (default 0.0)
    void setIsoLevel(float isoLevel) { m_isoLevel = isoLevel; }

    /// Extract mesh from the entire voxel grid
    Mesh extract(const VoxelGrid& grid) const;

    /// Extract mesh from a subregion (for partial updates)
    Mesh extractRegion(const VoxelGrid& grid,
                       int xMin, int yMin, int zMin,
                       int xMax, int yMax, int zMax) const;

private:
    float m_isoLevel = 0.0f;

    /// Interpolate vertex position along an edge
    Vec3 interpolateEdge(const Vec3& p1, const Vec3& p2,
                         float v1, float v2) const;

    /// Edge table: for each of 256 cube configurations, which edges are intersected
    static const int EDGE_TABLE[256];

    /// Triangle table: for each configuration, which edge triplets form triangles
    static const int TRI_TABLE[256][16];
};

} // namespace sky
