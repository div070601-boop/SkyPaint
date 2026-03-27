#pragma once

#include "../math/MathTypes.h"
#include <string>

namespace feather {

enum class ObjectType {
    STROKE,
    PRIMITIVE,
    SCULPT
};

struct SceneObject {
    int id = -1;
    Mesh mesh;
    Mat4 transform = Mat4(1.0f);
    Vec4 color = Vec4(1.0f);
    std::string name = "Object";
    int groupId = 0;
    bool visible = true;
    bool locked = false;
    ObjectType type = ObjectType::PRIMITIVE;
    int primitiveType = 0; // Maps to PrimitiveType enum (0=CUBE, 1=SPHERE, etc.)
};

} // namespace feather
