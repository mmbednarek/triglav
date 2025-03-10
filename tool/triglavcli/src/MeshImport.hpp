#pragma once

#include "triglav/io/Path.hpp"

namespace triglav::geometry {
class Mesh;
}

namespace triglav::tool::cli {

struct MeshImportProps
{
   io::Path srcPath;
   io::Path dstPath;
   bool shouldOverride{};
};

bool write_mesh_to_file(const geometry::Mesh& mesh, const io::Path& dstPath);
[[nodiscard]] bool import_mesh(const MeshImportProps& props);

}
