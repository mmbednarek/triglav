#include "Commands.hpp"

#include "LevelImport.hpp"
#include "MeshImport.hpp"
#include "TextureImport.hpp"
#include "triglav/ResourcePathMap.hpp"

#include "triglav/asset/Asset.hpp"
#include "triglav/gltf/Glb.hpp"
#include "triglav/project/PathManager.hpp"
#include "triglav/project/ProjectManager.hpp"

#include <format>
#include <iostream>

namespace triglav::tool::cli {

using namespace name_literals;

namespace {

std::string create_dst_resource_path(const project::ProjectMetadata& project_metadata, const io::Path& src_path,
                                     const ResourceType res_type)
{
   auto basename = src_path.basename();
   auto dot_at = basename.find_last_of('.');
   if (dot_at != std::string::npos) {
      basename = basename.substr(0, dot_at);
   }

   return project_metadata.default_import_path(res_type, basename);
}

std::optional<asset::TexturePurpose> parse_texture_purpose(const std::string_view purpose_str)
{
   if (purpose_str.empty()) {
      std::print(std::cerr, "warning: no texture purpose provided defaulting to albedo\n");
      return asset::TexturePurpose::Albedo;
   }

   if (purpose_str == "albedo") {
      return asset::TexturePurpose::Albedo;
   }
   if (purpose_str == "albedo_alpha") {
      return asset::TexturePurpose::AlbedoWithAlpha;
   }
   if (purpose_str == "normal_map") {
      return asset::TexturePurpose::NormalMap;
   }
   if (purpose_str == "bump_map") {
      return asset::TexturePurpose::BumpMap;
   }
   if (purpose_str == "metallic") {
      return asset::TexturePurpose::Metallic;
   }
   if (purpose_str == "roughness") {
      return asset::TexturePurpose::Roughness;
   }

   std::print(std::cerr, "error: unknown texture purpose '{}'\n", purpose_str);
   return std::nullopt;
}

ExitStatus handle_level_from_glb(const CmdArgs_import& args)
{
   const auto project_info = project::ProjectManager::the().active_project_info();
   if (project_info == nullptr) {
      std::print(std::cerr, "triglav-cli: No active project found\n");
      return EXIT_FAILURE;
   }

   const auto project_md = project::ProjectManager::the().active_project_metadata();
   if (project_md == nullptr) {
      std::print(std::cerr, "triglav-cli: No active project found\n");
      return EXIT_FAILURE;
   }

   auto sub_path = args.output_path.empty() ? create_dst_resource_path(*project_md, io::Path{args.positional_args[0]}, ResourceType::Level)
                                            : args.output_path;

   const LevelImportProps import_props{
      .src_path = io::Path{args.positional_args[0]},
      .dst_path = project::PathManager::the().translate_path(name_from_path(sub_path)),
      .should_override = args.should_override,
   };
   if (!import_level(import_props)) {
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}

ExitStatus handle_mesh_import(const CmdArgs_import& args)
{
   const auto project_md = project::ProjectManager::the().active_project_metadata();
   if (project_md == nullptr) {
      std::print(std::cerr, "triglav-cli: No active project found\n");
      return EXIT_FAILURE;
   }

   const auto sub_path = args.output_path.empty()
                            ? create_dst_resource_path(*project_md, io::Path{args.positional_args[0]}, ResourceType::Mesh)
                            : args.output_path;

   const MeshImportProps props{
      .src_path = io::Path{args.positional_args[0]},
      .dst_path = project::PathManager::the().translate_path(name_from_path(sub_path)),
      .should_override = args.should_override,
   };
   if (!import_mesh(props)) {
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}

std::optional<asset::SamplerProperties> parse_sampler_properties(const std::vector<std::string>& sampler_opts)
{
   asset::SamplerProperties sampler_properties;
   for (const auto& option : sampler_opts) {
      const auto sep = option.find('=');
      if (sep == std::string::npos)
         continue;

      const auto key = option.substr(0, sep);
      const auto value = option.substr(sep + 1);

      if (key == "minfilter") {
         auto filter = asset::filter_type_from_string(value);
         if (!filter.has_value()) {
            std::print(std::cerr, "invalid filter type: '{}'\n", value);
            return std::nullopt;
         }
         sampler_properties.min_filter = filter.value();
      } else if (key == "magfilter") {
         auto filter = asset::filter_type_from_string(option.substr(sep + 1));
         if (!filter.has_value()) {
            std::print(std::cerr, "invalid filter type: '{}'\n", value);
            return std::nullopt;
         }
         sampler_properties.mag_filter = filter.value();
      } else if (key == "addressmode.u") {
         auto address_mode = asset::texture_address_mode_from_string(option.substr(sep + 1));
         if (!address_mode.has_value()) {
            std::print(std::cerr, "invalid address mode: '{}'\n", value);
            return std::nullopt;
         }
         sampler_properties.address_mode_u = address_mode.value();
      } else if (key == "addressmode.v") {
         auto address_mode = asset::texture_address_mode_from_string(option.substr(sep + 1));
         if (!address_mode.has_value()) {
            std::print(std::cerr, "invalid address mode: '{}'\n", value);
            return std::nullopt;
         }
         sampler_properties.address_mode_v = address_mode.value();
      } else if (key == "addressmode.w") {
         auto address_mode = asset::texture_address_mode_from_string(option.substr(sep + 1));
         if (!address_mode.has_value()) {
            std::print(std::cerr, "invalid address mode: '{}'\n", value);
            return std::nullopt;
         }
         sampler_properties.address_mode_w = address_mode.value();
      } else if (key == "anisotropy") {
         sampler_properties.enable_anisotropy = option.substr(sep + 1) == "true" ? true : false;
      } else {
         std::print(std::cerr, "invalid sampler option key: '{}'\n", key);
         return std::nullopt;
      }
   }

   return sampler_properties;
}

ExitStatus handle_texture_import(const CmdArgs_import& args)
{
   const auto project_md = project::ProjectManager::the().active_project_metadata();
   if (project_md == nullptr) {
      std::print(std::cerr, "triglav-cli: No active project found\n");
      return EXIT_FAILURE;
   }

   auto sub_path = args.output_path.empty()
                      ? create_dst_resource_path(*project_md, io::Path{args.positional_args[0]}, ResourceType::Texture)
                      : args.output_path;
   auto dst_path = project::PathManager::the().translate_path(name_from_path(sub_path));

   const auto purpose = parse_texture_purpose(args.texture_purpose);
   if (!purpose.has_value()) {
      return EXIT_FAILURE;
   }

   const auto sampler_props = parse_sampler_properties(args.sampler_options);
   if (!sampler_props.has_value()) {
      return EXIT_FAILURE;
   }

   TextureImportProps import_props{
      .src_path = io::Path{args.positional_args[0]},
      .dst_path = project::PathManager::the().translate_path(name_from_path(sub_path)),
      .purpose = purpose.value(),
      .sampler_properties = *sampler_props,
      .should_compress = args.should_compress,
      .has_mip_maps = !args.no_mip_maps,
      .should_override = args.should_override,
   };
   if (!import_texture(import_props)) {
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}

}// namespace

ExitStatus handle_import(const CmdArgs_import& args)
{
   if (args.positional_args.size() != 1) {
      std::print(std::cerr, "must provide an input file\n");
      return EXIT_SUCCESS;
   }

   const auto src_file = args.positional_args[0];
   if (src_file.ends_with(".glb")) {
      return handle_level_from_glb(args);
   }
   if (src_file.ends_with(".obj")) {
      return handle_mesh_import(args);
   }
   if (src_file.ends_with(".jpg") || src_file.ends_with(".png")) {
      return handle_texture_import(args);
   }
   return EXIT_FAILURE;
}

}// namespace triglav::tool::cli