#include "Commands.hpp"

#include "triglav/ResourcePathMap.hpp"
#include "triglav/asset/Asset.hpp"
#include "triglav/project/PathManager.hpp"
#include "triglav/project/ProjectManager.hpp"

#include <cassert>
#include <iostream>
#include <print>

namespace triglav::tool::cli {

using namespace name_literals;

ExitStatus reimport_mesh(const std::string& rc_path, const size_t range_index, const StringView path)
{
   const auto sys_path = project::PathManager::the().translate_path(name_from_path(rc_path));

   std::optional<geometry::MeshData> mesh{};
   {
      const auto mesh_file_handle = io::open_file(sys_path, io::FileMode::Read);
      assert(mesh_file_handle.has_value());

      [[maybe_unused]]
      const auto asset_header = asset::decode_header(**mesh_file_handle);
      assert(asset_header.has_value());
      assert(asset_header->type == ResourceType::Mesh);

      mesh = asset::decode_mesh(**mesh_file_handle, asset_header->version);
   }
   assert(mesh.has_value());

   if (!path.is_empty() && range_index < mesh->vertex_data.ranges.size()) {
      mesh->vertex_data.ranges[range_index].material_name = name_from_path(path);
   }

   const auto mesh_file_output = io::open_file(sys_path, io::FileMode::Write);
   if (!mesh_file_output.has_value()) {
      std::println(std::cerr, "triglav-cli: Failed to open file for writing");
      return 1;
   }
   asset::encode_mesh_data(**mesh_file_output, mesh.value());
   return 0;
}

ExitStatus handle_reimport(const CmdArgs_reimport& args)
{
   if (args.positional_args.empty()) {
      std::println(std::cerr, "triglav-cli: Missing assert path");
      return 1;
   }

   const auto& rc_path = args.positional_args.at(0);
   if (rc_path.ends_with(".mesh")) {
      reimport_mesh(rc_path, args.range_index, StringView{args.material});
      return 0;
   }

   std::print(std::cerr, "triglav-cli: Unsupported resource type\n");
   return 1;
}

}// namespace triglav::tool::cli
