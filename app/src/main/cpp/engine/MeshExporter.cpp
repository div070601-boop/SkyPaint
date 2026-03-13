#include "MeshExporter.h"
#include <fstream>
#include <sstream>
#include <cstring>

namespace feather {

// ── OBJ Export ──────────────────────────────────────────────────────────────

std::string MeshExporter::exportOBJ(const Mesh& mesh, const std::string& objectName) const {
    std::ostringstream ss;
    ss << "# Feather 3D Export\n";
    ss << "# Vertices: " << mesh.vertices.size() << "\n";
    ss << "# Faces: " << mesh.indices.size() / 3 << "\n\n";
    ss << "o " << objectName << "\n\n";

    // Vertices
    for (const auto& v : mesh.vertices) {
        ss << "v " << v.position.x << " " << v.position.y << " " << v.position.z << "\n";
    }
    ss << "\n";

    // Normals
    for (const auto& v : mesh.vertices) {
        ss << "vn " << v.normal.x << " " << v.normal.y << " " << v.normal.z << "\n";
    }
    ss << "\n";

    // Texture coords
    for (const auto& v : mesh.vertices) {
        ss << "vt " << v.uv.x << " " << v.uv.y << "\n";
    }
    ss << "\n";

    // Faces (OBJ is 1-indexed)
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        uint32_t i0 = mesh.indices[i] + 1;
        uint32_t i1 = mesh.indices[i + 1] + 1;
        uint32_t i2 = mesh.indices[i + 2] + 1;
        ss << "f " << i0 << "/" << i0 << "/" << i0
           << " " << i1 << "/" << i1 << "/" << i1
           << " " << i2 << "/" << i2 << "/" << i2 << "\n";
    }

    return ss.str();
}

std::string MeshExporter::exportOBJ(const std::vector<Mesh>& meshes,
                                     const std::string& baseName) const {
    std::ostringstream ss;
    ss << "# Feather 3D Export\n\n";

    uint32_t vertexOffset = 0;
    for (size_t m = 0; m < meshes.size(); ++m) {
        const auto& mesh = meshes[m];
        ss << "o " << baseName << "_" << m << "\n";

        for (const auto& v : mesh.vertices) {
            ss << "v " << v.position.x << " " << v.position.y << " " << v.position.z << "\n";
        }
        for (const auto& v : mesh.vertices) {
            ss << "vn " << v.normal.x << " " << v.normal.y << " " << v.normal.z << "\n";
        }
        for (const auto& v : mesh.vertices) {
            ss << "vt " << v.uv.x << " " << v.uv.y << "\n";
        }
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            uint32_t i0 = mesh.indices[i] + vertexOffset + 1;
            uint32_t i1 = mesh.indices[i + 1] + vertexOffset + 1;
            uint32_t i2 = mesh.indices[i + 2] + vertexOffset + 1;
            ss << "f " << i0 << "/" << i0 << "/" << i0
               << " " << i1 << "/" << i1 << "/" << i1
               << " " << i2 << "/" << i2 << "/" << i2 << "\n";
        }
        ss << "\n";
        vertexOffset += static_cast<uint32_t>(mesh.vertices.size());
    }

    return ss.str();
}

bool MeshExporter::exportOBJToFile(const Mesh& mesh, const std::string& filePath,
                                    const std::string& objectName) const {
    std::ofstream file(filePath);
    if (!file.is_open()) return false;
    file << exportOBJ(mesh, objectName);
    return file.good();
}

// ── glTF Binary (GLB) Export ────────────────────────────────────────────────

// Helper: write little-endian uint32 to buffer
static void writeU32(std::vector<uint8_t>& buf, uint32_t value) {
    buf.push_back(value & 0xFF);
    buf.push_back((value >> 8) & 0xFF);
    buf.push_back((value >> 16) & 0xFF);
    buf.push_back((value >> 24) & 0xFF);
}

static void writeFloat(std::vector<uint8_t>& buf, float value) {
    uint8_t bytes[4];
    std::memcpy(bytes, &value, 4);
    buf.insert(buf.end(), bytes, bytes + 4);
}

// Pad buffer to 4-byte alignment
static void padTo4(std::vector<uint8_t>& buf) {
    while (buf.size() % 4 != 0) {
        buf.push_back(0x20); // space for JSON chunk, 0x00 for BIN
    }
}

std::vector<uint8_t> MeshExporter::exportGLB(const Mesh& mesh,
                                              const std::string& objectName) const {
    // Build binary buffer: positions | normals | uvs | indices
    std::vector<uint8_t> binBuffer;

    // Calculate sizes
    uint32_t numVertices = static_cast<uint32_t>(mesh.vertices.size());
    uint32_t numIndices = static_cast<uint32_t>(mesh.indices.size());
    uint32_t posSize = numVertices * 3 * sizeof(float);
    uint32_t normalSize = numVertices * 3 * sizeof(float);
    uint32_t uvSize = numVertices * 2 * sizeof(float);
    uint32_t indexSize = numIndices * sizeof(uint32_t);

    uint32_t posOffset = 0;
    uint32_t normalOffset = posSize;
    uint32_t uvOffset = normalOffset + normalSize;
    uint32_t indexOffset = uvOffset + uvSize;
    uint32_t totalBinSize = indexOffset + indexSize;

    binBuffer.reserve(totalBinSize);

    // Compute bounds for accessor min/max
    Vec3 posMin(std::numeric_limits<float>::max());
    Vec3 posMax(std::numeric_limits<float>::lowest());

    // Write positions
    for (const auto& v : mesh.vertices) {
        writeFloat(binBuffer, v.position.x);
        writeFloat(binBuffer, v.position.y);
        writeFloat(binBuffer, v.position.z);
        posMin = glm::min(posMin, v.position);
        posMax = glm::max(posMax, v.position);
    }

    // Write normals
    for (const auto& v : mesh.vertices) {
        writeFloat(binBuffer, v.normal.x);
        writeFloat(binBuffer, v.normal.y);
        writeFloat(binBuffer, v.normal.z);
    }

    // Write UVs
    for (const auto& v : mesh.vertices) {
        writeFloat(binBuffer, v.uv.x);
        writeFloat(binBuffer, v.uv.y);
    }

    // Write indices
    for (uint32_t idx : mesh.indices) {
        writeU32(binBuffer, idx);
    }

    // Pad binary buffer
    while (binBuffer.size() % 4 != 0) binBuffer.push_back(0x00);

    // Build JSON
    std::ostringstream json;
    json << "{";
    json << "\"asset\":{\"version\":\"2.0\",\"generator\":\"Feather3D\"},";
    json << "\"scene\":0,";
    json << "\"scenes\":[{\"nodes\":[0]}],";
    json << "\"nodes\":[{\"mesh\":0,\"name\":\"" << objectName << "\"}],";
    json << "\"meshes\":[{\"primitives\":[{";
    json << "\"attributes\":{\"POSITION\":0,\"NORMAL\":1,\"TEXCOORD_0\":2},";
    json << "\"indices\":3";
    json << "}]}],";
    json << "\"accessors\":[";
    // 0: positions
    json << "{\"bufferView\":0,\"componentType\":5126,\"count\":" << numVertices
         << ",\"type\":\"VEC3\""
         << ",\"min\":[" << posMin.x << "," << posMin.y << "," << posMin.z << "]"
         << ",\"max\":[" << posMax.x << "," << posMax.y << "," << posMax.z << "]},";
    // 1: normals
    json << "{\"bufferView\":1,\"componentType\":5126,\"count\":" << numVertices
         << ",\"type\":\"VEC3\"},";
    // 2: uvs
    json << "{\"bufferView\":2,\"componentType\":5126,\"count\":" << numVertices
         << ",\"type\":\"VEC2\"},";
    // 3: indices
    json << "{\"bufferView\":3,\"componentType\":5125,\"count\":" << numIndices
         << ",\"type\":\"SCALAR\"}";
    json << "],";
    json << "\"bufferViews\":[";
    json << "{\"buffer\":0,\"byteOffset\":" << posOffset << ",\"byteLength\":" << posSize << "},";
    json << "{\"buffer\":0,\"byteOffset\":" << normalOffset << ",\"byteLength\":" << normalSize << "},";
    json << "{\"buffer\":0,\"byteOffset\":" << uvOffset << ",\"byteLength\":" << uvSize << "},";
    json << "{\"buffer\":0,\"byteOffset\":" << indexOffset << ",\"byteLength\":" << indexSize
         << ",\"target\":34963}";
    json << "],";
    json << "\"buffers\":[{\"byteLength\":" << binBuffer.size() << "}]";
    json << "}";

    std::string jsonStr = json.str();

    // Pad JSON to 4 bytes
    while (jsonStr.size() % 4 != 0) jsonStr.push_back(' ');

    // Build GLB
    std::vector<uint8_t> glb;
    uint32_t totalSize = 12 + // GLB header
                         8 + static_cast<uint32_t>(jsonStr.size()) + // JSON chunk
                         8 + static_cast<uint32_t>(binBuffer.size()); // BIN chunk

    // GLB Header
    writeU32(glb, 0x46546C67); // magic: "glTF"
    writeU32(glb, 2);           // version
    writeU32(glb, totalSize);   // total length

    // JSON Chunk
    writeU32(glb, static_cast<uint32_t>(jsonStr.size()));
    writeU32(glb, 0x4E4F534A); // "JSON"
    glb.insert(glb.end(), jsonStr.begin(), jsonStr.end());

    // BIN Chunk
    writeU32(glb, static_cast<uint32_t>(binBuffer.size()));
    writeU32(glb, 0x004E4942); // "BIN\0"
    glb.insert(glb.end(), binBuffer.begin(), binBuffer.end());

    return glb;
}

std::vector<uint8_t> MeshExporter::exportGLB(const std::vector<Mesh>& meshes,
                                              const std::string& baseName) const {
    // Combine all meshes into one for GLB export
    Mesh combined;
    for (const auto& mesh : meshes) {
        uint32_t offset = static_cast<uint32_t>(combined.vertices.size());
        combined.vertices.insert(combined.vertices.end(),
                                 mesh.vertices.begin(), mesh.vertices.end());
        for (uint32_t idx : mesh.indices) {
            combined.indices.push_back(idx + offset);
        }
    }
    return exportGLB(combined, baseName);
}

bool MeshExporter::exportGLBToFile(const Mesh& mesh, const std::string& filePath,
                                    const std::string& objectName) const {
    auto data = exportGLB(mesh, objectName);
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) return false;
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

} // namespace feather
