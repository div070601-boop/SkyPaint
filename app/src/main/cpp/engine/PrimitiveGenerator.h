#pragma once

#include "../math/MathTypes.h"

namespace sky {

enum class PrimitiveType {
    CUBE,
    SPHERE,
    CYLINDER,
    CONE,
    PLANE,
    TORUS
};

class PrimitiveGenerator {
public:
    static Mesh generateCube(float size = 1.0f);
    static Mesh generateSphere(float radius = 0.5f, int slices = 32, int stacks = 16);
    static Mesh generateCylinder(float radius = 0.5f, float height = 1.0f, int slices = 32);
    static Mesh generateCone(float radius = 0.5f, float height = 1.0f, int slices = 32);
    static Mesh generatePlane(float width = 1.0f, float depth = 1.0f, int subdivisionsX = 1, int subdivisionsZ = 1);
    static Mesh generateTorus(float outerRadius = 0.5f, float innerRadius = 0.2f, int radialSegments = 32, int tubularSegments = 16);
};

} // namespace sky
