#include "Commands.hpp"
#include "ProjectConfig.hpp"
#include "ResourceList.hpp"
#include "triglav/ResourcePathMap.hpp"

#include "triglav/asset/Asset.hpp"

#include <cassert>
#include <iostream>
#include <map>
#include <print>

namespace triglav::tool::cli {

// std::map<std::string, std::string> g_translate_path;
ExitStatus reimport_mesh(const ProjectInfo& info, const std::string& rc_path)
{
   const auto sys_path = info.content_path(rc_path);

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

   auto project_info = load_active_project_info();
   if (!project_info.has_value()) {
      std::print(std::cerr, "triglav-cli: No active project found\n");
      return EXIT_FAILURE;
   }

   const auto index_path = project_info->content_path("index.yaml");
   auto res_list = ResourceList::from_file(index_path);
   assert(res_list.has_value());

   for (const auto& [name, item] : res_list->resources) {
      ResourcePathMap::the().store_path(StringView{name.data(), name.size()});
   }

   const auto& rc_path = args.positional_args.at(0);
   if (rc_path.ends_with(".mesh")) {
      reimport_mesh(*project_info, rc_path);
      return 0;
   }

   std::print(std::cerr, "triglav-cli: Unsupported resource type\n");
   return 1;
}

}// namespace triglav::tool::cli
