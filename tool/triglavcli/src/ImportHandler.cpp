#include "Commands.hpp"

#include "triglav/asset/Asset.hpp"
#include "triglav/gltf/MeshLoad.hpp"
#include "triglav/io/File.hpp"

#include <fmt/core.h>

namespace triglav::tool::cli {

ExitStatus handle_import(const ImportArgs& args)
{
   const auto mesh = gltf::load_glb_mesh(io::Path(args.srcAsset));
   if (!mesh.has_value()) {
      fmt::print(stderr, "Failed to load glb mesh\n");
      return EXIT_FAILURE;
   }

   const auto outFile = io::open_file(io::Path(args.dstPath), io::FileOpenMode::Create);
   if (!outFile.has_value()) {
      fmt::print(stderr, "Failed to open output file\n");
      return EXIT_FAILURE;
   }

   if (!asset::encode_mesh(*mesh, **outFile)) {
      fmt::print(stderr, "Failed to encode mesh\n");
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}

}// namespace triglav::tool::cli