#include "MeshImport.hpp"

#include "triglav/asset/Asset.hpp"
#include "triglav/geometry/Mesh.hpp"
#include "triglav/gltf/MeshLoad.hpp"
#include "triglav/io/File.hpp"

#include <fmt/core.h>
#include <optional>

namespace triglav::tool::cli {

namespace {

std::optional<geometry::Mesh> load_mesh(const io::Path& srcAsset)
{
   if (srcAsset.string().ends_with(".obj")) {
      return geometry::Mesh::from_file(io::Path{srcAsset});
   }
   if (srcAsset.string().ends_with(".glb")) {
      return gltf::load_glb_mesh(io::Path(srcAsset));
   }
   return std::nullopt;
}
}// namespace

bool write_mesh_to_file(const geometry::Mesh& mesh, const io::Path& dstPath)
{
   mesh.triangulate();
   mesh.recalculate_tangents();

   const auto outFile = io::open_file(dstPath, io::FileOpenMode::Create);
   if (!outFile.has_value()) {
      fmt::print(stderr, "Failed to open output file\n");
      return false;
   }

   if (!asset::encode_mesh(**outFile, mesh)) {
      fmt::print(stderr, "Failed to encode mesh\n");
      return false;
   }

   return true;
}

bool import_mesh(const MeshImportProps& props)
{
   if (!props.shouldOverride && props.dstPath.exists()) {
      fmt::print(stderr, "triglav-cli: Failed to import mesh to {}, file exists", props.dstPath.string());
      return false;
   }

   const auto mesh = load_mesh(props.srcPath);
   if (!mesh.has_value()) {
      fmt::print(stderr, "Failed to load glb mesh\n");
      return EXIT_FAILURE;
   }

   return write_mesh_to_file(*mesh, props.dstPath);
}

}// namespace triglav::tool::cli
