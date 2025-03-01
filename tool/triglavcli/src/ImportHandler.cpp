#include "Commands.hpp"

#include "ProjectConfig.hpp"

#include "triglav/asset/Asset.hpp"
#include "triglav/gltf/MeshLoad.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/Instance.hpp"
#include "triglav/io/File.hpp"
#include "triglav/ktx/Texture.hpp"

#include <fmt/core.h>
#include <stbi/stb_image.h>

namespace triglav::tool::cli {

namespace {

std::string create_dst_resource_path(const ProjectInfo& projectInfo, const io::Path& srcPath, const ResourceType resType)
{
   auto basename = srcPath.basename();
   auto dotAt = basename.find_last_of('.');
   if (dotAt != std::string::npos) {
      basename = basename.substr(0, dotAt);
   }

   switch (resType) {
   case ResourceType::Texture:
      return fmt::format(fmt::runtime(projectInfo.importSettings.texturePath), fmt::arg("basename", basename));
   case ResourceType::Mesh:
      return fmt::format(fmt::runtime(projectInfo.importSettings.meshPath), fmt::arg("basename", basename));
   default:
      break;
   }

   return {};
}

io::Path create_destination_path(const ProjectInfo& projectInfo, const std::string_view srcPath, const std::string_view outputPath,
                                 const ResourceType resType)
{
   std::string dstResourcePath{outputPath};
   if (dstResourcePath.empty()) {
      dstResourcePath = create_dst_resource_path(projectInfo, io::Path{srcPath}, resType);
   }
   if (io::path_seperator() != '/') {
      for (auto& ch : dstResourcePath) {
         if (ch == '/') {
            ch = io::path_seperator();
         }
      }
   }

   auto dstPath = io::Path{projectInfo.path};
   dstPath = dstPath.sub(dstResourcePath);

   return dstPath;
}

std::optional<asset::TexturePurpose> parse_texture_purpose(const std::string_view purposeStr)
{
   if (purposeStr.empty()) {
      fmt::print(stderr, "warning: no texture purpose provided defaulting to albedo\n");
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

   fmt::print(stderr, "error: unknown texture purpose '{}'\n", purposeStr);
   return std::nullopt;
}

std::optional<geometry::Mesh> load_mesh(const std::string_view srcAsset)
{
   if (srcAsset.ends_with(".obj")) {
      return geometry::Mesh::from_file(io::Path{srcAsset});
   }
   if (srcAsset.ends_with(".glb")) {
      return gltf::load_glb_mesh(io::Path(srcAsset));
   }
   return std::nullopt;
}

ExitStatus handle_mesh_import(const CmdArgs_import& args)
{
   auto projectInfo = load_active_project_info();
   if (!projectInfo.has_value()) {
      fmt::print(stderr, "triglav-cli: No active project found\n");
      return EXIT_FAILURE;
   }

   auto dstPath = create_destination_path(*projectInfo, args.positionalArgs[0], args.outputPath, ResourceType::Texture);

   const auto mesh = load_mesh(args.positionalArgs[0]);
   if (!mesh.has_value()) {
      fmt::print(stderr, "Failed to load glb mesh\n");
      return EXIT_FAILURE;
   }

   mesh->triangulate();
   mesh->recalculate_tangents();

   const auto outFile = io::open_file(dstPath, io::FileOpenMode::Create);
   if (!outFile.has_value()) {
      fmt::print(stderr, "Failed to open output file\n");
      return EXIT_FAILURE;
   }

   if (!asset::encode_mesh(**outFile, *mesh)) {
      fmt::print(stderr, "Failed to encode mesh\n");
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
            fmt::print(stderr, "invalid filter type: '{}'\n", value);
            return std::nullopt;
         }
         samplerProperties.minFilter = filter.value();
      } else if (key == "magfilter") {
         auto filter = asset::filter_type_from_string(option.substr(sep + 1));
         if (!filter.has_value()) {
            fmt::print(stderr, "invalid filter type: '{}'\n", value);
            return std::nullopt;
         }
         samplerProperties.magFilter = filter.value();
      } else if (key == "addressmode.u") {
         auto addressMode = asset::texture_address_mode_from_string(option.substr(sep + 1));
         if (!addressMode.has_value()) {
            fmt::print(stderr, "invalid address mode: '{}'\n", value);
            return std::nullopt;
         }
         samplerProperties.addressModeU = addressMode.value();
      } else if (key == "addressmode.v") {
         auto addressMode = asset::texture_address_mode_from_string(option.substr(sep + 1));
         if (!addressMode.has_value()) {
            fmt::print(stderr, "invalid address mode: '{}'\n", value);
            return std::nullopt;
         }
         samplerProperties.addressModeV = addressMode.value();
      } else if (key == "addressmode.w") {
         auto addressMode = asset::texture_address_mode_from_string(option.substr(sep + 1));
         if (!addressMode.has_value()) {
            fmt::print(stderr, "invalid address mode: '{}'\n", value);
            return std::nullopt;
         }
         samplerProperties.addressModeW = addressMode.value();
      } else if (key == "anisotropy") {
         samplerProperties.enableAnisotropy = option.substr(sep + 1) == "true" ? true : false;
      } else {
         fmt::print(stderr, "invalid sampler option key: '{}'\n", key);
         return std::nullopt;
      }
   }

   return samplerProperties;
}

ExitStatus handle_texture_import(const CmdArgs_import& args)
{
   auto projectInfo = load_active_project_info();
   if (!projectInfo.has_value()) {
      fmt::print(stderr, "triglav-cli: No active project found\n");
      return EXIT_FAILURE;
   }

   auto dstPath = create_destination_path(*projectInfo, args.positionalArgs[0], args.outputPath, ResourceType::Texture);

   fmt::print(stderr, "triglav-cli: Importing texture to {}\n", dstPath.string());

   const auto purpose = parse_texture_purpose(args.texturePurpose);
   if (!purpose.has_value()) {
      return EXIT_FAILURE;
   }

   int texWidth, texHeight, texChannels;
   stbi_uc* pixels = stbi_load(args.positionalArgs[0].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
   if (pixels == nullptr) {
      fmt::print(stderr, "triglav-cli: Source texture not found '{}'\n", args.positionalArgs[0].c_str());
      return EXIT_FAILURE;
   }

   const auto instance = GAPI_CHECK(graphics_api::Instance::create_instance());
   auto device = GAPI_CHECK(instance.create_device(nullptr, graphics_api::DevicePickStrategy::PreferDedicated, 0));

   auto format = GAPI_FORMAT(RGBA, UNorm8);
   if (*purpose == asset::TexturePurpose::Albedo || *purpose == asset::TexturePurpose::AlbedoWithAlpha) {
      format = GAPI_FORMAT(RGBA, sRGB);
   }

   auto texture = GAPI_CHECK(device->create_texture(
      format, {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)},
      graphics_api::TextureUsage::Sampled | graphics_api::TextureUsage::TransferDst | graphics_api::TextureUsage::TransferSrc,
      graphics_api::TextureState::Undefined, graphics_api::SampleCount::Single, args.noMipMaps ? 1 : graphics_api::g_maxMipMaps));

   GAPI_CHECK_STATUS(texture.write(*device, pixels));

   stbi_image_free(pixels);

   auto ktxTexture = GAPI_CHECK(device->export_ktx_texture(texture));

   if (args.shouldCompress) {
      // TODO: Figure out how to transcode with isNormalMap = true.
      if (!ktxTexture.compress(ktx::TextureCompression::UASTC, false)) {
         return EXIT_FAILURE;
      }

      ktx::TextureTranscode transcode{};
      switch (*purpose) {
      case asset::TexturePurpose::Albedo:
         transcode = ktx::TextureTranscode::BC1_RGB;
         break;
      case asset::TexturePurpose::AlbedoWithAlpha:
         transcode = ktx::TextureTranscode::BC3_RGBA;
         break;
      case asset::TexturePurpose::NormalMap:
         transcode = ktx::TextureTranscode::BC1_RGB;
         break;
      default:
         transcode = ktx::TextureTranscode::BC4_R;
         break;
      }

      if (!ktxTexture.transcode(transcode)) {
         return EXIT_FAILURE;
      }
   }

   const auto outFile = io::open_file(dstPath, io::FileOpenMode::Create);
   if (!outFile.has_value()) {
      return EXIT_FAILURE;
   }

   const auto samplerData = parse_sampler_properties(args.samplerOptions);
   if (!samplerData.has_value()) {
      return EXIT_FAILURE;
   }

   if (!asset::encode_texture(**outFile, *purpose, ktxTexture, *samplerData)) {
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}

}// namespace

ExitStatus handle_import(const CmdArgs_import& args)
{
   if (args.positionalArgs.size() != 1) {
      fmt::print(stderr, "must provide an input file\n");
      return EXIT_SUCCESS;
   }

   const auto srcFile = args.positionalArgs[0];
   if (srcFile.ends_with(".obj") || srcFile.ends_with(".glb")) {
      return handle_mesh_import(args);
   }
   if (srcFile.ends_with(".jpg") || srcFile.ends_with(".png")) {
      return handle_texture_import(args);
   }
   return EXIT_FAILURE;
}

}// namespace triglav::tool::cli