#include "Commands.hpp"

#include "triglav/asset/Asset.hpp"
#include "triglav/io/File.hpp"

#include <fmt/core.h>
#include <triglav/ktx/Texture.hpp>

namespace triglav::tool::cli {

std::string_view asset_type_to_string(const ResourceType type)
{
   switch (type) {
   case ResourceType::Texture:
      return "texture";
   case ResourceType::Mesh:
      return "mesh";
   default:
      break;
   }
   return "unsupported";
}

std::string_view texture_purpose_to_string(const asset::TexturePurpose purpose)
{
   switch (purpose) {
   case asset::TexturePurpose::Albedo:
      return "albedo";
   case asset::TexturePurpose::AlbedoWithAlpha:
      return "albedo_alpha";
   case asset::TexturePurpose::NormalMap:
      return "normal_map";
   case asset::TexturePurpose::BumpMap:
      return "bump_map";
   case asset::TexturePurpose::Metallic:
      return "metallic";
   case asset::TexturePurpose::Roughness:
      return "roughness";
   case asset::TexturePurpose::Mask:
      return "mask";
   default:
      break;
   }

   return "";
}

ExitStatus handle_inspect(const CmdArgs_inspect& args)
{
   if (args.positionalArgs.empty()) {
      return EXIT_FAILURE;
   }

   const auto fileHandle = io::open_file(io::Path(args.positionalArgs[0]), io::FileOpenMode::Read);
   if (!fileHandle.has_value()) {
      return EXIT_FAILURE;
   }

   auto header = asset::decode_header(**fileHandle);
   if (!header.has_value()) {
      return EXIT_FAILURE;
   }

   fmt::print("TRIGLAV ASSET FILE\n");
   fmt::print("asset type: {}\n\n", asset_type_to_string(header->type));

   if (header->type == ResourceType::Mesh) {
      auto mesh = asset::decode_mesh(**fileHandle);
      if (!mesh.has_value()) {
         return EXIT_FAILURE;
      }

      fmt::print("vertex count: {}\n", mesh->vertexData.vertices.size());
      fmt::print("index count: {}\n", mesh->vertexData.indices.size());
      fmt::print("bounding box min: ({}, {}, {})\n", mesh->boundingBox.min.x, mesh->boundingBox.min.y, mesh->boundingBox.min.z);
      fmt::print("bounding box max: ({}, {}, {})\n", mesh->boundingBox.max.x, mesh->boundingBox.max.y, mesh->boundingBox.max.z);
   }
   if (header->type == ResourceType::Texture) {
      auto decodedTex = asset::decode_texture(**fileHandle);
      if (!decodedTex.has_value()) {
         fmt::print(stderr, "failed to decode texture\n");
         return EXIT_FAILURE;
      }

      const auto dims = decodedTex->texture.dimensions();

      fmt::print("purpose: {}\n", texture_purpose_to_string(decodedTex->purpose));
      fmt::print("magfilter: {}\n", asset::filter_type_to_string(decodedTex->samplerProps.magFilter));
      fmt::print("minfilter: {}\n", asset::filter_type_to_string(decodedTex->samplerProps.minFilter));
      fmt::print("addressmod.u: {}\n", asset::texture_address_mode_to_string(decodedTex->samplerProps.addressModeU));
      fmt::print("addressmod.v: {}\n", asset::texture_address_mode_to_string(decodedTex->samplerProps.addressModeV));
      fmt::print("addressmod.w: {}\n", asset::texture_address_mode_to_string(decodedTex->samplerProps.addressModeW));
      fmt::print("anisotropy: {}\n", decodedTex->samplerProps.enableAnisotropy ? "true" : "false");
      fmt::print("dimensions: {} {}\n", dims.x, dims.y);
   }

   return EXIT_SUCCESS;
}

}// namespace triglav::tool::cli
