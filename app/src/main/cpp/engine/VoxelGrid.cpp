#include "VoxelGrid.h"

namespace feather {

void VoxelGrid::initialize(int resolution, const Vec3& boundsMin, const Vec3& boundsMax) {
    m_resolution = resolution;
    m_boundsMin = boundsMin;
    m_boundsMax = boundsMax;
    m_cellSize = (boundsMax - boundsMin) / static_cast<float>(resolution - 1);

    size_t totalSize = static_cast<size_t>(resolution) * resolution * resolution;
    m_field.resize(totalSize, 1.0f); // positive = outside surface

    clearDirty();
}

float VoxelGrid::getValue(int x, int y, int z) const {
    if (!inBounds(x, y, z)) return 1.0f;
    return m_field[index(x, y, z)];
}

void VoxelGrid::setValue(int x, int y, int z, float value) {
    if (!inBounds(x, y, z)) return;
    m_field[index(x, y, z)] = value;
}

Vec3 VoxelGrid::gridToWorld(int x, int y, int z) const {
    return m_boundsMin + Vec3(
        static_cast<float>(x) * m_cellSize.x,
        static_cast<float>(y) * m_cellSize.y,
        static_cast<float>(z) * m_cellSize.z
    );
}

Vec3 VoxelGrid::worldToGrid(const Vec3& worldPos) const {
    return (worldPos - m_boundsMin) / m_cellSize;
}

void VoxelGrid::addSphere(const Vec3& center, float radius, float strength) {
    Vec3 gridCenter = worldToGrid(center);
    float gridRadius = radius / m_cellSize.x; // assume uniform grid

    int minX = std::max(0, static_cast<int>(gridCenter.x - gridRadius) - 1);
    int minY = std::max(0, static_cast<int>(gridCenter.y - gridRadius) - 1);
    int minZ = std::max(0, static_cast<int>(gridCenter.z - gridRadius) - 1);
    int maxX = std::min(m_resolution - 1, static_cast<int>(gridCenter.x + gridRadius) + 1);
    int maxY = std::min(m_resolution - 1, static_cast<int>(gridCenter.y + gridRadius) + 1);
    int maxZ = std::min(m_resolution - 1, static_cast<int>(gridCenter.z + gridRadius) + 1);

    for (int z = minZ; z <= maxZ; ++z) {
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                Vec3 worldPos = gridToWorld(x, y, z);
                float dist = glm::distance(worldPos, center);
                float sphereSDF = dist - radius;

                // Smooth union
                float existing = getValue(x, y, z);
                float blended = std::min(existing, sphereSDF * strength);
                setValue(x, y, z, blended);
            }
        }
    }

    markDirty(minX, minY, minZ, maxX, maxY, maxZ);
}

void VoxelGrid::subtractSphere(const Vec3& center, float radius, float strength) {
    Vec3 gridCenter = worldToGrid(center);
    float gridRadius = radius / m_cellSize.x;

    int minX = std::max(0, static_cast<int>(gridCenter.x - gridRadius) - 1);
    int minY = std::max(0, static_cast<int>(gridCenter.y - gridRadius) - 1);
    int minZ = std::max(0, static_cast<int>(gridCenter.z - gridRadius) - 1);
    int maxX = std::min(m_resolution - 1, static_cast<int>(gridCenter.x + gridRadius) + 1);
    int maxY = std::min(m_resolution - 1, static_cast<int>(gridCenter.y + gridRadius) + 1);
    int maxZ = std::min(m_resolution - 1, static_cast<int>(gridCenter.z + gridRadius) + 1);

    for (int z = minZ; z <= maxZ; ++z) {
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                Vec3 worldPos = gridToWorld(x, y, z);
                float dist = glm::distance(worldPos, center);
                float sphereSDF = -(dist - radius); // negate for subtraction

                float existing = getValue(x, y, z);
                float blended = std::max(existing, sphereSDF * strength);
                setValue(x, y, z, blended);
            }
        }
    }

    markDirty(minX, minY, minZ, maxX, maxY, maxZ);
}

void VoxelGrid::smooth(const Vec3& center, float radius, int iterations) {
    Vec3 gridCenter = worldToGrid(center);
    float gridRadius = radius / m_cellSize.x;

    int minX = std::max(1, static_cast<int>(gridCenter.x - gridRadius) - 1);
    int minY = std::max(1, static_cast<int>(gridCenter.y - gridRadius) - 1);
    int minZ = std::max(1, static_cast<int>(gridCenter.z - gridRadius) - 1);
    int maxX = std::min(m_resolution - 2, static_cast<int>(gridCenter.x + gridRadius) + 1);
    int maxY = std::min(m_resolution - 2, static_cast<int>(gridCenter.y + gridRadius) + 1);
    int maxZ = std::min(m_resolution - 2, static_cast<int>(gridCenter.z + gridRadius) + 1);

    std::vector<float> temp(m_field.size());

    for (int iter = 0; iter < iterations; ++iter) {
        // Copy current field state for this iteration
        std::copy(m_field.begin(), m_field.end(), temp.begin());

        for (int z = minZ; z <= maxZ; ++z) {
            for (int y = minY; y <= maxY; ++y) {
                for (int x = minX; x <= maxX; ++x) {
                    Vec3 worldPos = gridToWorld(x, y, z);
                    float dist = glm::distance(worldPos, center);
                    if (dist > radius) continue;

                    float weight = 1.0f - (dist / radius);
                    weight = weight * weight; // smooth falloff

                    // 6-neighbor average
                    float avg = (
                        getValue(x - 1, y, z) +
                        getValue(x + 1, y, z) +
                        getValue(x, y - 1, z) +
                        getValue(x, y + 1, z) +
                        getValue(x, y, z - 1) +
                        getValue(x, y, z + 1)
                    ) / 6.0f;

                    float current = getValue(x, y, z);
                    temp[index(x, y, z)] = glm::mix(current, avg, weight);
                }
            }
        }

        m_field = temp;
    }

    markDirty(minX, minY, minZ, maxX, maxY, maxZ);
}

void VoxelGrid::markDirty(int xMin, int yMin, int zMin, int xMax, int yMax, int zMax) {
    if (!m_isDirty) {
        m_dirtyMin[0] = xMin;
        m_dirtyMin[1] = yMin;
        m_dirtyMin[2] = zMin;
        m_dirtyMax[0] = xMax;
        m_dirtyMax[1] = yMax;
        m_dirtyMax[2] = zMax;
        m_isDirty = true;
    } else {
        m_dirtyMin[0] = std::min(m_dirtyMin[0], xMin);
        m_dirtyMin[1] = std::min(m_dirtyMin[1], yMin);
        m_dirtyMin[2] = std::min(m_dirtyMin[2], zMin);
        m_dirtyMax[0] = std::max(m_dirtyMax[0], xMax);
        m_dirtyMax[1] = std::max(m_dirtyMax[1], yMax);
        m_dirtyMax[2] = std::max(m_dirtyMax[2], zMax);
    }
}

bool VoxelGrid::getDirtyBounds(int& xMin, int& yMin, int& zMin, int& xMax, int& yMax, int& zMax) {
    if (!m_isDirty) return false;
    xMin = m_dirtyMin[0];
    yMin = m_dirtyMin[1];
    zMin = m_dirtyMin[2];
    xMax = m_dirtyMax[0];
    yMax = m_dirtyMax[1];
    zMax = m_dirtyMax[2];
    return true;
}

void VoxelGrid::clearDirty() {
    m_isDirty = false;
    m_dirtyMin[0] = m_dirtyMin[1] = m_dirtyMin[2] = 0;
    m_dirtyMax[0] = m_dirtyMax[1] = m_dirtyMax[2] = 0;
}

} // namespace feather
