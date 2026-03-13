#pragma once

#include "../math/MathTypes.h"
#include <string>

namespace feather {

/// Exports meshes to OBJ and glTF 2.0 binary formats
class MeshExporter {
public:
    MeshExporter() = default;

    /// Export mesh to OBJ format string
    std::string exportOBJ(const Mesh& mesh, const std::string& objectName = "Feather3D") const;

    /// Export mesh to OBJ file
    bool exportOBJToFile(const Mesh& mesh, const std::string& filePath,
                         const std::string& objectName = "Feather3D") const;

    /// Export mesh to glTF 2.0 binary (.glb) buffer
    std::vector<uint8_t> exportGLB(const Mesh& mesh,
                                    const std::string& objectName = "Feather3D") const;

    /// Export mesh to glTF binary file
    bool exportGLBToFile(const Mesh& mesh, const std::string& filePath,
                         const std::string& objectName = "Feather3D") const;

    /// Export multiple meshes combined
    std::string exportOBJ(const std::vector<Mesh>& meshes,
                          const std::string& baseName = "Feather3D") const;

    std::vector<uint8_t> exportGLB(const std::vector<Mesh>& meshes,
                                    const std::string& baseName = "Feather3D") const;
};

} // namespace feather
