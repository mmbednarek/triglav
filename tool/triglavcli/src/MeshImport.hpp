#pragma once

#include "triglav/io/Path.hpp"

namespace triglav::geometry {
class Mesh;
}

namespace triglav::tool::cli {

struct MeshImportProps
{
   io::Path src_path;
   io::Path dst_path;
   bool should_override{};
};

bool write_mesh_to_file(const geometry::Mesh& mesh, const io::Path& dst_path);
[[nodiscard]] bool import_mesh(const MeshImportProps& props);

}// namespace triglav::tool::cli
