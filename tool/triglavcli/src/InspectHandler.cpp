#include "Commands.hpp"

#include "triglav/asset/Asset.hpp"
#include "triglav/io/File.hpp"

#include <print>
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
   if (args.positional_args.empty()) {
      return EXIT_FAILURE;
   }

   const auto file_handle = io::open_file(io::Path(args.positional_args[0]), io::FileMode::Read);
   if (!file_handle.has_value()) {
      return EXIT_FAILURE;
   }

   auto header = asset::decode_header(**file_handle);
   if (!header.has_value()) {
      return EXIT_FAILURE;
   }

   std::print("TRIGLAV ASSET FILE\n");
   std::print("asset type: {}\n\n", asset_type_to_string(header->type));

   if (header->type == ResourceType::Mesh) {
      auto mesh = asset::decode_mesh(**file_handle, header->version);
      if (!mesh.has_value()) {
         return EXIT_FAILURE;
      }

      std::print("vertex count: {}\n", mesh->vertex_data.vertices.size());
      std::print("index count: {}\n", mesh->vertex_data.indices.size());
      std::print("bounding box min: ({}, {}, {})\n", mesh->bounding_box.min.x, mesh->bounding_box.min.y, mesh->bounding_box.min.z);
      std::print("bounding box max: ({}, {}, {})\n", mesh->bounding_box.max.x, mesh->bounding_box.max.y, mesh->bounding_box.max.z);
   }
   if (header->type == ResourceType::Texture) {
      auto decoded_tex = asset::decode_texture(**file_handle);
      if (!decoded_tex.has_value()) {
         std::print(stderr, "failed to decode texture\n");
         return EXIT_FAILURE;
      }

      const auto dims = decoded_tex->texture.dimensions();

      std::print("purpose: {}\n", texture_purpose_to_string(decoded_tex->purpose));
      std::print("magfilter: {}\n", asset::filter_type_to_string(decoded_tex->sampler_props.mag_filter));
      std::print("minfilter: {}\n", asset::filter_type_to_string(decoded_tex->sampler_props.min_filter));
      std::print("addressmod.u: {}\n", asset::texture_address_mode_to_string(decoded_tex->sampler_props.address_mode_u));
      std::print("addressmod.v: {}\n", asset::texture_address_mode_to_string(decoded_tex->sampler_props.address_mode_v));
      std::print("addressmod.w: {}\n", asset::texture_address_mode_to_string(decoded_tex->sampler_props.address_mode_w));
      std::print("anisotropy: {}\n", decoded_tex->sampler_props.enable_anisotropy ? "true" : "false");
      std::print("dimensions: {} {}\n", dims.x, dims.y);
   }

   return EXIT_SUCCESS;
}

}// namespace triglav::tool::cli
