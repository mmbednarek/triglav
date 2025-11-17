#include "MeshImport.hpp"

#include "triglav/asset/Asset.hpp"
#include "triglav/geometry/Mesh.hpp"
#include "triglav/gltf/MeshLoad.hpp"
#include "triglav/io/File.hpp"

#include <optional>
#include <print>

namespace triglav::tool::cli {

namespace {

std::optional<geometry::Mesh> load_mesh(const io::Path& src_asset)
{
   if (src_asset.string().ends_with(".obj")) {
      return geometry::Mesh::from_file(io::Path{src_asset});
   }
   if (src_asset.string().ends_with(".glb")) {
      return gltf::load_glb_mesh(io::Path(src_asset));
   }
   return std::nullopt;
}
}// namespace

bool write_mesh_to_file(const geometry::Mesh& mesh, const io::Path& dst_path)
{
   mesh.triangulate();
   mesh.recalculate_tangents();

   const auto out_file = io::open_file(dst_path, io::FileOpenMode::Create);
   if (!out_file.has_value()) {
      std::print(stderr, "Failed to open output file\n");
      return false;
   }

   if (!asset::encode_mesh(**out_file, mesh)) {
      std::print(stderr, "Failed to encode mesh\n");
      return false;
   }

   return true;
}

bool import_mesh(const MeshImportProps& props)
{
   if (!props.should_override && props.dst_path.exists()) {
      std::print(stderr, "triglav-cli: Failed to import mesh to {}, file exists", props.dst_path.string());
      return false;
   }

   const auto mesh = load_mesh(props.src_path);
   if (!mesh.has_value()) {
      std::print(stderr, "Failed to load glb mesh\n");
      return EXIT_FAILURE;
   }

   return write_mesh_to_file(*mesh, props.dst_path);
}

}// namespace triglav::tool::cli
