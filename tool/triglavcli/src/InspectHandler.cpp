#include "Commands.hpp"

#include "triglav/asset/Asset.hpp"
#include "triglav/io/File.hpp"

#include <fmt/core.h>

namespace triglav::tool::cli {

ExitStatus handle_inspect(const InspectArgs& args)
{
   const auto fileHandle = io::open_file(io::Path(args.inputAccess), io::FileOpenMode::Read);
   if (!fileHandle.has_value()) {
      return EXIT_FAILURE;
   }

   auto mesh = asset::decode_mesh(**fileHandle);
   if (!mesh.has_value()) {
      return EXIT_FAILURE;
   }

   fmt::print("TRIGLAV ASSET FILE\n\n");
   fmt::print("resource type: mesh\n");
   fmt::print("vertex count: {}\n", mesh->vertexData.vertices.size());
   fmt::print("index count: {}\n", mesh->vertexData.indices.size());
   fmt::print("bounding box min: ({}, {}, {})\n", mesh->boundingBox.min.x, mesh->boundingBox.min.y, mesh->boundingBox.min.z);
   fmt::print("bounding box max: ({}, {}, {})\n", mesh->boundingBox.max.x, mesh->boundingBox.max.y, mesh->boundingBox.max.z);

   return EXIT_SUCCESS;
}

}// namespace triglav::tool::cli
