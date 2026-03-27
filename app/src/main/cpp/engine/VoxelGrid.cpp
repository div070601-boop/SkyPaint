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

void VoxelGrid::mergePrimitiveSDF(int primitiveType, const Mat4& transform, bool isSubtract) {
    // 1. Compute AABB of the transformed primitive
    Vec3 corners[8];
    int idx = 0;
    float r = 0.5f; // base primitive bounds are roughly [-0.5, 0.5]
    if (primitiveType == 5) r = 0.65f; // Torus is slightly larger

    for(int i=0; i<2; i++)
    for(int j=0; j<2; j++)
    for(int k=0; k<2; k++) {
        Vec3 local = Vec3(i?r:-r, j?r:-r, k?r:-r);
        Vec4 worldObj = transform * Vec4(local.x, local.y, local.z, 1.0f);
        corners[idx++] = Vec3(worldObj.x, worldObj.y, worldObj.z);
    }

    Vec3 wMin = corners[0];
    Vec3 wMax = corners[0];
    for(int i=1; i<8; i++) {
        wMin.x = std::min(wMin.x, corners[i].x);
        wMin.y = std::min(wMin.y, corners[i].y);
        wMin.z = std::min(wMin.z, corners[i].z);
        wMax.x = std::max(wMax.x, corners[i].x);
        wMax.y = std::max(wMax.y, corners[i].y);
        wMax.z = std::max(wMax.z, corners[i].z);
    }

    // Add margin
    float margin = m_cellSize.x * 4.0f;
    wMin -= Vec3(margin);
    wMax += Vec3(margin);

    Vec3 gMin = worldToGrid(wMin);
    Vec3 gMax = worldToGrid(wMax);

    int minX = std::max(0, static_cast<int>(gMin.x));
    int minY = std::max(0, static_cast<int>(gMin.y));
    int minZ = std::max(0, static_cast<int>(gMin.z));
    int maxX = std::min(m_resolution - 1, static_cast<int>(gMax.x) + 1);
    int maxY = std::min(m_resolution - 1, static_cast<int>(gMax.y) + 1);
    int maxZ = std::min(m_resolution - 1, static_cast<int>(gMax.z) + 1);

    Mat4 invTransform = glm::inverse(transform);
    // Approximate scale factor to adjust distances
    float scaleApprox = glm::length(Vec3(transform[0][0], transform[0][1], transform[0][2]));
    if (scaleApprox < 0.001f) scaleApprox = 1.0f;

    for (int z = minZ; z <= maxZ; ++z) {
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                Vec3 worldPos = gridToWorld(x, y, z);
                Vec4 localPos4 = invTransform * Vec4(worldPos.x, worldPos.y, worldPos.z, 1.0f);
                Vec3 p = Vec3(localPos4.x, localPos4.y, localPos4.z);

                float d = 1.0f;
                // SDF evaluation by type:
                // 0: Cube, 1: Sphere, 2: Cylinder, 3: Cone, 4: Plane, 5: Torus
                if (primitiveType == 0) {
                    Vec3 q = glm::abs(p) - Vec3(0.5f);
                    d = glm::length(glm::max(q, 0.0f)) + std::min(std::max(q.x, std::max(q.y, q.z)), 0.0f);
                } else if (primitiveType == 1) {
                    d = glm::length(p) - 0.5f;
                } else if (primitiveType == 2) {
                    glm::vec2 d2 = glm::abs(glm::vec2(glm::length(glm::vec2(p.x, p.z)), p.y)) - glm::vec2(0.5f, 0.5f);
                    d = std::min(std::max(d2.x, d2.y), 0.0f) + glm::length(glm::max(d2, 0.0f));
                } else if (primitiveType == 4) {
                    Vec3 q = glm::abs(p) - Vec3(0.5f, 0.02f, 0.5f);
                    d = glm::length(glm::max(q, 0.0f)) + std::min(std::max(q.x, std::max(q.y, q.z)), 0.0f);
                } else if (primitiveType == 5) {
                    glm::vec2 q = glm::vec2(glm::length(glm::vec2(p.x, p.z)) - 0.5f, p.y);
                    d = glm::length(q) - 0.15f;
                } else {
                    // Cone or unknown
                    d = glm::length(p) - 0.5f;
                }

                // Global distance
                d *= scaleApprox;

                if (isSubtract) d = -d;

                float existing = getValue(x, y, z);
                float blended = isSubtract ? std::max(existing, d) : std::min(existing, d);
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
