#include "Commands.hpp"

#include "LevelImport.hpp"
#include "MeshImport.hpp"
#include "ProjectConfig.hpp"
#include "ResourceList.hpp"
#include "TextureImport.hpp"

#include "triglav/asset/Asset.hpp"
#include "triglav/gltf/Glb.hpp"
#include "triglav/gltf/MeshLoad.hpp"
#include "triglav/world/Level.hpp"

#include <algorithm>
#include <format>
#include <iostream>

namespace triglav::tool::cli {

using namespace name_literals;

namespace {

std::string create_dst_resource_path(const ProjectInfo& projectInfo, const io::Path& srcPath, const ResourceType resType)
{
   auto basename = srcPath.basename();
   auto dotAt = basename.find_last_of('.');
   if (dotAt != std::string::npos) {
      basename = basename.substr(0, dotAt);
   }

   return projectInfo.default_import_path(resType, basename);
}

std::optional<asset::TexturePurpose> parse_texture_purpose(const std::string_view purposeStr)
{
   if (purposeStr.empty()) {
      std::print(std::cerr, "warning: no texture purpose provided defaulting to albedo\n");
      return asset::TexturePurpose::Albedo;
   }

   if (purposeStr == "albedo") {
      return asset::TexturePurpose::Albedo;
   }
   if (purposeStr == "albedo_alpha") {
      return asset::TexturePurpose::AlbedoWithAlpha;
   }
   if (purposeStr == "normal_map") {
      return asset::TexturePurpose::NormalMap;
   }
   if (purposeStr == "bump_map") {
      return asset::TexturePurpose::BumpMap;
   }
   if (purposeStr == "metallic") {
      return asset::TexturePurpose::Metallic;
   }
   if (purposeStr == "roughness") {
      return asset::TexturePurpose::Roughness;
   }

   std::print(std::cerr, "error: unknown texture purpose '{}'\n", purposeStr);
   return std::nullopt;
}

ExitStatus handle_level_from_glb(const CmdArgs_import& args)
{
   auto projectInfo = load_active_project_info();
   if (!projectInfo.has_value()) {
      std::print(std::cerr, "triglav-cli: No active project found\n");
      return EXIT_FAILURE;
   }

   auto subPath = args.outputPath.empty() ? create_dst_resource_path(*projectInfo, io::Path{args.positionalArgs[0]}, ResourceType::Level)
                                          : args.outputPath;

   const LevelImportProps importProps{
      .srcPath = io::Path{args.positionalArgs[0]},
      .dstPath = projectInfo->content_path(subPath),
      .shouldOverride = args.shouldOverride,
   };
   if (!import_level(importProps)) {
      return EXIT_FAILURE;
   }

   if (!add_resource_to_index(subPath)) {
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}

ExitStatus handle_mesh_import(const CmdArgs_import& args)
{
   auto projectInfo = load_active_project_info();
   if (!projectInfo.has_value()) {
      std::print(std::cerr, "triglav-cli: No active project found\n");
      return EXIT_FAILURE;
   }

   const auto subPath = args.outputPath.empty()
                           ? create_dst_resource_path(*projectInfo, io::Path{args.positionalArgs[0]}, ResourceType::Mesh)
                           : args.outputPath;

   const MeshImportProps props{
      .srcPath = io::Path{args.positionalArgs[0]},
      .dstPath = projectInfo->content_path(subPath),
      .shouldOverride = args.shouldOverride,
   };
   if (!import_mesh(props)) {
      return EXIT_FAILURE;
   }

   if (!add_resource_to_index(subPath)) {
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}

std::optional<asset::SamplerProperties> parse_sampler_properties(const std::vector<std::string>& samplerOpts)
{
   asset::SamplerProperties samplerProperties;
   for (const auto& option : samplerOpts) {
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
         samplerProperties.minFilter = filter.value();
      } else if (key == "magfilter") {
         auto filter = asset::filter_type_from_string(option.substr(sep + 1));
         if (!filter.has_value()) {
            std::print(std::cerr, "invalid filter type: '{}'\n", value);
            return std::nullopt;
         }
         samplerProperties.magFilter = filter.value();
      } else if (key == "addressmode.u") {
         auto addressMode = asset::texture_address_mode_from_string(option.substr(sep + 1));
         if (!addressMode.has_value()) {
            std::print(std::cerr, "invalid address mode: '{}'\n", value);
            return std::nullopt;
         }
         samplerProperties.addressModeU = addressMode.value();
      } else if (key == "addressmode.v") {
         auto addressMode = asset::texture_address_mode_from_string(option.substr(sep + 1));
         if (!addressMode.has_value()) {
            std::print(std::cerr, "invalid address mode: '{}'\n", value);
            return std::nullopt;
         }
         samplerProperties.addressModeV = addressMode.value();
      } else if (key == "addressmode.w") {
         auto addressMode = asset::texture_address_mode_from_string(option.substr(sep + 1));
         if (!addressMode.has_value()) {
            std::print(std::cerr, "invalid address mode: '{}'\n", value);
            return std::nullopt;
         }
         samplerProperties.addressModeW = addressMode.value();
      } else if (key == "anisotropy") {
         samplerProperties.enableAnisotropy = option.substr(sep + 1) == "true" ? true : false;
      } else {
         std::print(std::cerr, "invalid sampler option key: '{}'\n", key);
         return std::nullopt;
      }
   }

   return samplerProperties;
}

ExitStatus handle_texture_import(const CmdArgs_import& args)
{
   auto projectInfo = load_active_project_info();
   if (!projectInfo.has_value()) {
      std::print(std::cerr, "triglav-cli: No active project found\n");
      return EXIT_FAILURE;
   }

   auto subPath = args.outputPath.empty() ? create_dst_resource_path(*projectInfo, io::Path{args.positionalArgs[0]}, ResourceType::Texture)
                                          : args.outputPath;
   auto dstPath = projectInfo->content_path(subPath);

   const auto purpose = parse_texture_purpose(args.texturePurpose);
   if (!purpose.has_value()) {
      return EXIT_FAILURE;
   }

   const auto samplerProps = parse_sampler_properties(args.samplerOptions);
   if (!samplerProps.has_value()) {
      return EXIT_FAILURE;
   }

   TextureImportProps importProps{
      .srcPath = io::Path{args.positionalArgs[0]},
      .dstPath = projectInfo->content_path(subPath),
      .purpose = purpose.value(),
      .samplerProperties = *samplerProps,
      .shouldCompress = args.shouldCompress,
      .hasMipMaps = !args.noMipMaps,
      .shouldOverride = args.shouldOverride,
   };
   if (!import_texture(importProps)) {
      return EXIT_FAILURE;
   }

   if (!add_resource_to_index(subPath)) {
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}

}// namespace

ExitStatus handle_import(const CmdArgs_import& args)
{
   if (args.positionalArgs.size() != 1) {
      std::print(std::cerr, "must provide an input file\n");
      return EXIT_SUCCESS;
   }

   const auto srcFile = args.positionalArgs[0];
   if (srcFile.ends_with(".glb")) {
      return handle_level_from_glb(args);
   }
   if (srcFile.ends_with(".obj")) {
      return handle_mesh_import(args);
   }
   if (srcFile.ends_with(".jpg") || srcFile.ends_with(".png")) {
      return handle_texture_import(args);
   }
   return EXIT_FAILURE;
}

}// namespace triglav::tool::cli