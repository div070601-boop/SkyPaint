#include "PrimitiveGenerator.h"
#include <cmath>

namespace feather {

Mesh PrimitiveGenerator::generateCube(float size) {
    Mesh mesh;
    float half = size * 0.5f;

    // Define 8 unique corners
    Vec3 p[8] = {
        {-half, -half,  half}, // 0: front-bottom-left
        { half, -half,  half}, // 1: front-bottom-right
        { half,  half,  half}, // 2: front-top-right
        {-half,  half,  half}, // 3: front-top-left
        {-half, -half, -half}, // 4: back-bottom-left
        { half, -half, -half}, // 5: back-bottom-right
        { half,  half, -half}, // 6: back-top-right
        {-half,  half, -half}  // 7: back-top-left
    };

    // 6 faces * 4 vertices = 24 vertices
    // We duplicate vertices to have sharp flat normals per face
    Vec3 n[6] = {
        { 0,  0,  1}, // Front
        { 0,  0, -1}, // Back
        {-1,  0,  0}, // Left
        { 1,  0,  0}, // Right
        { 0,  1,  0}, // Top
        { 0, -1,  0}  // Bottom
    };

    int faceIndices[6][4] = {
        {0, 1, 2, 3}, // Front
        {5, 4, 7, 6}, // Back
        {4, 0, 3, 7}, // Left
        {1, 5, 6, 2}, // Right
        {3, 2, 6, 7}, // Top
        {4, 5, 1, 0}  // Bottom
    };

    Vec2 uvs[4] = {
        {0, 0}, {1, 0}, {1, 1}, {0, 1}
    };

    for (int i = 0; i < 6; ++i) {
        uint32_t startIndex = static_cast<uint32_t>(mesh.vertices.size());
        
        for (int j = 0; j < 4; ++j) {
            Vertex v;
            v.position = p[faceIndices[i][j]];
            v.normal = n[i];
            v.uv = uvs[j];
            v.color = Vec4(1.0f);
            mesh.vertices.push_back(v);
        }

        mesh.indices.push_back(startIndex + 0);
        mesh.indices.push_back(startIndex + 1);
        mesh.indices.push_back(startIndex + 2);
        mesh.indices.push_back(startIndex + 0);
        mesh.indices.push_back(startIndex + 2);
        mesh.indices.push_back(startIndex + 3);
    }

    return mesh;
}

Mesh PrimitiveGenerator::generateSphere(float radius, int slices, int stacks) {
    Mesh mesh;

    for (int i = 0; i <= stacks; ++i) {
        float v = (float)i / stacks;
        float phi = v * M_PI;

        for (int j = 0; j <= slices; ++j) {
            float u = (float)j / slices;
            float theta = u * 2.0f * M_PI;

            float x = cos(theta) * sin(phi);
            float y = cos(phi);
            float z = sin(theta) * sin(phi);

            Vertex vert;
            vert.normal = Vec3(x, y, z);
            vert.position = vert.normal * radius;
            vert.uv = Vec2(u, v);
            vert.color = Vec4(1.0f);
            mesh.vertices.push_back(vert);
        }
    }

    // Indices
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            uint32_t first = (i * (slices + 1)) + j;
            uint32_t second = first + slices + 1;

            mesh.indices.push_back(first);
            mesh.indices.push_back(second);
            mesh.indices.push_back(first + 1);

            mesh.indices.push_back(second);
            mesh.indices.push_back(second + 1);
            mesh.indices.push_back(first + 1);
        }
    }

    return mesh;
}

Mesh PrimitiveGenerator::generateCylinder(float radius, float height, int slices) {
    Mesh mesh;
    float halfHeight = height * 0.5f;

    // Side wall vertices
    uint32_t sideStart = 0;
    for (int i = 0; i <= slices; ++i) {
        float u = (float)i / slices;
        float theta = u * 2.0f * M_PI;
        
        float x = cos(theta);
        float z = sin(theta);
        
        // Bottom
        Vertex vb;
        vb.position = Vec3(x * radius, -halfHeight, z * radius);
        vb.normal = Vec3(x, 0, z);
        vb.uv = Vec2(u, 0.0f);
        vb.color = Vec4(1.0f);
        mesh.vertices.push_back(vb);

        // Top
        Vertex vt;
        vt.position = Vec3(x * radius, halfHeight, z * radius);
        vt.normal = Vec3(x, 0, z);
        vt.uv = Vec2(u, 1.0f);
        vt.color = Vec4(1.0f);
        mesh.vertices.push_back(vt);
    }

    for (int i = 0; i < slices; ++i) {
        uint32_t first = i * 2;
        mesh.indices.push_back(first);
        mesh.indices.push_back(first + 1);
        mesh.indices.push_back(first + 2);
        
        mesh.indices.push_back(first + 1);
        mesh.indices.push_back(first + 3);
        mesh.indices.push_back(first + 2);
    }

    // Top and Bottom Caps
    uint32_t topCenter = mesh.vertices.size();
    Vertex vtc; vtc.position = Vec3(0, halfHeight, 0); vtc.normal = Vec3(0, 1, 0); vtc.uv = Vec2(0.5f); vtc.color = Vec4(1.0f);
    mesh.vertices.push_back(vtc);
    
    uint32_t topStart = mesh.vertices.size();
    for (int i = 0; i <= slices; ++i) {
        float u = (float)i / slices;
        float theta = u * 2.0f * M_PI;
        Vertex v; v.position = Vec3(cos(theta)*radius, halfHeight, sin(theta)*radius);
        v.normal = Vec3(0, 1, 0); v.uv = Vec2(cos(theta)*0.5f+0.5f, sin(theta)*0.5f+0.5f); v.color = Vec4(1.0f);
        mesh.vertices.push_back(v);
    }
    for (int i = 0; i < slices; ++i) {
        mesh.indices.push_back(topCenter);
        mesh.indices.push_back(topStart + i + 1);
        mesh.indices.push_back(topStart + i);
    }

    uint32_t bottomCenter = mesh.vertices.size();
    Vertex vbc; vbc.position = Vec3(0, -halfHeight, 0); vbc.normal = Vec3(0, -1, 0); vbc.uv = Vec2(0.5f); vbc.color = Vec4(1.0f);
    mesh.vertices.push_back(vbc);
    
    uint32_t bottomStart = mesh.vertices.size();
    for (int i = 0; i <= slices; ++i) {
        float u = (float)i / slices;
        float theta = u * 2.0f * M_PI;
        Vertex v; v.position = Vec3(cos(theta)*radius, -halfHeight, sin(theta)*radius);
        v.normal = Vec3(0, -1, 0); v.uv = Vec2(cos(theta)*0.5f+0.5f, sin(theta)*0.5f+0.5f); v.color = Vec4(1.0f);
        mesh.vertices.push_back(v);
    }
    for (int i = 0; i < slices; ++i) {
        mesh.indices.push_back(bottomCenter);
        mesh.indices.push_back(bottomStart + i);
        mesh.indices.push_back(bottomStart + i + 1);
    }

    return mesh;
}

Mesh PrimitiveGenerator::generateCone(float radius, float height, int slices) {
    Mesh mesh;
    float halfHeight = height * 0.5f;

    // Side wall
    uint32_t topVert = mesh.vertices.size();
    Vertex vTop; vTop.position = Vec3(0, halfHeight, 0); vTop.normal = Vec3(0, 1, 0); vTop.uv = Vec2(0.5f, 1.0f); vTop.color = Vec4(1.0f);
    mesh.vertices.push_back(vTop); // Shared top vertex, though for proper smooth shading it should have matching normals (here rough)

    uint32_t baseStart = mesh.vertices.size();
    float slantAngle = atan2(height, radius); // normal pointing up and out
    
    for (int i = 0; i <= slices; ++i) {
        float u = (float)i / slices;
        float theta = u * 2.0f * M_PI;
        float nx = cos(theta) * sin(slantAngle);
        float ny = cos(slantAngle);
        float nz = sin(theta) * sin(slantAngle);

        Vertex vSideDrop; 
        vSideDrop.position = Vec3(0, halfHeight, 0);
        vSideDrop.normal = Vec3(nx, ny, nz);
        vSideDrop.uv = Vec2(u, 1.0f);
        vSideDrop.color = Vec4(1.0f);
        
        Vertex vBase; 
        vBase.position = Vec3(cos(theta)*radius, -halfHeight, sin(theta)*radius);
        vBase.normal = Vec3(nx, ny, nz);
        vBase.uv = Vec2(u, 0.0f);
        vBase.color = Vec4(1.0f);
        
        mesh.vertices.push_back(vSideDrop);
        mesh.vertices.push_back(vBase);
    }

    for (int i = 0; i < slices; ++i) {
        uint32_t first = baseStart + (i * 2);
        mesh.indices.push_back(first);
        mesh.indices.push_back(first + 1);
        mesh.indices.push_back(first + 3);
    }

    // Base cap
    uint32_t bottomCenter = mesh.vertices.size();
    Vertex vbc; vbc.position = Vec3(0, -halfHeight, 0); vbc.normal = Vec3(0, -1, 0); vbc.uv = Vec2(0.5f); vbc.color = Vec4(1.0f);
    mesh.vertices.push_back(vbc);
    
    uint32_t capStart = mesh.vertices.size();
    for (int i = 0; i <= slices; ++i) {
        float u = (float)i / slices;
        float theta = u * 2.0f * M_PI;
        Vertex v; v.position = Vec3(cos(theta)*radius, -halfHeight, sin(theta)*radius);
        v.normal = Vec3(0, -1, 0); v.uv = Vec2(cos(theta)*0.5f+0.5f, sin(theta)*0.5f+0.5f); v.color = Vec4(1.0f);
        mesh.vertices.push_back(v);
    }
    for (int i = 0; i < slices; ++i) {
        mesh.indices.push_back(bottomCenter);
        mesh.indices.push_back(capStart + i);
        mesh.indices.push_back(capStart + i + 1);
    }

    return mesh;
}

Mesh PrimitiveGenerator::generatePlane(float width, float depth, int subdivisionsX, int subdivisionsZ) {
    Mesh mesh;
    float halfW = width * 0.5f;
    float halfD = depth * 0.5f;

    for (int z = 0; z <= subdivisionsZ; ++z) {
        float vz = (float)z / subdivisionsZ;
        for (int x = 0; x <= subdivisionsX; ++x) {
            float vx = (float)x / subdivisionsX;

            Vertex v;
            v.position = Vec3(-halfW + vx * width, 0.0f, -halfD + vz * depth);
            v.normal = Vec3(0, 1, 0);
            v.uv = Vec2(vx, vz);
            v.color = Vec4(1.0f);
            mesh.vertices.push_back(v);
        }
    }

    for (int z = 0; z < subdivisionsZ; ++z) {
        for (int x = 0; x < subdivisionsX; ++x) {
            uint32_t first = (z * (subdivisionsX + 1)) + x;
            uint32_t second = first + subdivisionsX + 1;

            mesh.indices.push_back(first);
            mesh.indices.push_back(second);
            mesh.indices.push_back(first + 1);

            mesh.indices.push_back(second);
            mesh.indices.push_back(second + 1);
            mesh.indices.push_back(first + 1);
        }
    }

    return mesh;
}

Mesh PrimitiveGenerator::generateTorus(float outerRadius, float innerRadius, int radialSegments, int tubularSegments) {
    Mesh mesh;

    for (int j = 0; j <= radialSegments; ++j) {
        float v = (float)j / radialSegments;
        float theta = v * 2.0f * M_PI;
        float cosTheta = cos(theta);
        float sinTheta = sin(theta);

        for (int i = 0; i <= tubularSegments; ++i) {
            float u = (float)i / tubularSegments;
            float phi = u * 2.0f * M_PI;
            float cosPhi = cos(phi);
            float sinPhi = sin(phi);

            // Torus parametric Equation
            float x = (outerRadius + innerRadius * cosPhi) * cosTheta;
            float y = innerRadius * sinPhi;
            float z = (outerRadius + innerRadius * cosPhi) * sinTheta;

            // Center of the tube for this slice
            float cx = outerRadius * cosTheta;
            float cy = 0.0f;
            float cz = outerRadius * sinTheta;

            Vertex vert;
            vert.position = Vec3(x, y, z);
            vert.normal = glm::normalize(Vec3(x - cx, y - cy, z - cz));
            vert.uv = Vec2(u, v);
            vert.color = Vec4(1.0f);
            mesh.vertices.push_back(vert);
        }
    }

    for (int j = 1; j <= radialSegments; ++j) {
        for (int i = 1; i <= tubularSegments; ++i) {
            uint32_t a = (tubularSegments + 1) * j + i - 1;
            uint32_t b = (tubularSegments + 1) * (j - 1) + i - 1;
            uint32_t c = (tubularSegments + 1) * (j - 1) + i;
            uint32_t d = (tubularSegments + 1) * j + i;

            mesh.indices.push_back(a);
            mesh.indices.push_back(b);
            mesh.indices.push_back(d);

            mesh.indices.push_back(b);
            mesh.indices.push_back(c);
            mesh.indices.push_back(d);
        }
    }

    return mesh;
}

} // namespace feather
