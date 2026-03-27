with open('app/src/main/cpp/engine/GeometryEngine.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

primitive_code = """
// ── Primitive Objects ───────────────────────────────────────────────────────

int GeometryEngine::addPrimitive(PrimitiveType type, const Mat4& transform, const Vec4& color) {
    SceneObject obj;
    obj.id = m_nextObjectId++;
    obj.transform = transform;
    obj.color = color;
    obj.type = ObjectType::PRIMITIVE;

    switch (type) {
        case PrimitiveType::CUBE:     obj.mesh = PrimitiveGenerator::generateCube(); break;
        case PrimitiveType::SPHERE:   obj.mesh = PrimitiveGenerator::generateSphere(); break;
        case PrimitiveType::CYLINDER: obj.mesh = PrimitiveGenerator::generateCylinder(); break;
        case PrimitiveType::CONE:     obj.mesh = PrimitiveGenerator::generateCone(); break;
        case PrimitiveType::PLANE:    obj.mesh = PrimitiveGenerator::generatePlane(); break;
        case PrimitiveType::TORUS:    obj.mesh = PrimitiveGenerator::generateTorus(); break;
    }

    m_primitiveObjects.push_back(obj);
    return obj.id;
}

const SceneObject* GeometryEngine::getPrimitive(int index) const {
    if (index >= 0 && index < static_cast<int>(m_primitiveObjects.size())) {
        return &m_primitiveObjects[index];
    }
    return nullptr;
}

int GeometryEngine::getPrimitiveCount() const {
    return static_cast<int>(m_primitiveObjects.size());
}

void GeometryEngine::removePrimitive(int index) {
    if (index >= 0 && index < static_cast<int>(m_primitiveObjects.size())) {
        m_primitiveObjects.erase(m_primitiveObjects.begin() + index);
    }
}

void GeometryEngine::clearPrimitives() {
    m_primitiveObjects.clear();
}

"""

# Insert right before '} // namespace sky'
content = content.replace('} // namespace sky', primitive_code + '} // namespace sky')

with open('app/src/main/cpp/engine/GeometryEngine.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("Done")
