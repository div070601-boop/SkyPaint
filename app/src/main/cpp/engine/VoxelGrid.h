#pragma once

#include "../math/MathTypes.h"

namespace feather {

/// SDF-based voxel grid for sculpting operations
class VoxelGrid {
public:
    VoxelGrid() = default;

    /// Initialize grid with given resolution and world-space bounds
    void initialize(int resolution, const Vec3& boundsMin, const Vec3& boundsMax);

    /// Get resolution
    int getResolution() const { return m_resolution; }

    /// Get voxel value at grid coordinates
    float getValue(int x, int y, int z) const;

    /// Set voxel value at grid coordinates
    void setValue(int x, int y, int z, float value);

    /// Add a sphere to the distance field (union)
    void addSphere(const Vec3& center, float radius, float strength = 1.0f);

    /// Subtract a sphere from the distance field
    void subtractSphere(const Vec3& center, float radius, float strength = 1.0f);

    /// Merge an arbitrary predefined primitive into the terrain (CSG Boolean)
    void mergePrimitiveSDF(int primitiveType, const Mat4& transform, bool isSubtract);

    /// Smooth the field in a region
    void smooth(const Vec3& center, float radius, int iterations = 1);

    /// Convert grid coordinates to world position
    Vec3 gridToWorld(int x, int y, int z) const;

    /// Convert world position to grid coordinates (float)
    Vec3 worldToGrid(const Vec3& worldPos) const;

    /// Get bounds
    const Vec3& getBoundsMin() const { return m_boundsMin; }
    const Vec3& getBoundsMax() const { return m_boundsMax; }

    /// Get raw field data
    const std::vector<float>& getField() const { return m_field; }

    /// Set raw field data (for undo/redo)
    void setField(const std::vector<float>& field) {
        if (field.size() == m_field.size()) {
            m_field = field;
            markDirty(0, 0, 0, m_resolution - 1, m_resolution - 1, m_resolution - 1);
        }
    }

    /// Check if grid is initialized
    bool isInitialized() const { return m_resolution > 0; }

    /// Mark dirty region for partial updates
    void markDirty(int xMin, int yMin, int zMin, int xMax, int yMax, int zMax);

    /// Get and clear dirty bounds
    bool getDirtyBounds(int& xMin, int& yMin, int& zMin, int& xMax, int& yMax, int& zMax);
    void clearDirty();

private:
    int m_resolution = 0;
    Vec3 m_boundsMin{0.0f};
    Vec3 m_boundsMax{1.0f};
    Vec3 m_cellSize{0.0f};
    std::vector<float> m_field;

    // Dirty tracking for partial updates
    bool m_isDirty = false;
    int m_dirtyMin[3] = {0, 0, 0};
    int m_dirtyMax[3] = {0, 0, 0};

    int index(int x, int y, int z) const {
        return x + y * m_resolution + z * m_resolution * m_resolution;
    }

    bool inBounds(int x, int y, int z) const {
        return x >= 0 && x < m_resolution &&
               y >= 0 && y < m_resolution &&
               z >= 0 && z < m_resolution;
    }
};

} // namespace feather
