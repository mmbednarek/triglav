#include "TextureImport.hpp"

#include "ProjectConfig.hpp"

#include "triglav/graphics_api/Instance.hpp"
#include "triglav/io/File.hpp"

#include <fmt/core.h>
#include <stbi/stb_image.h>

namespace triglav::tool::cli {

namespace {

graphics_api::ColorFormat format_from_purpose(const asset::TexturePurpose purpose)
{
   if (purpose == asset::TexturePurpose::Albedo || purpose == asset::TexturePurpose::AlbedoWithAlpha) {
      return GAPI_FORMAT(RGBA, sRGB);
   }
   return GAPI_FORMAT(RGBA, UNorm8);
}

ktx::TextureTranscode transcode_from_purpose(const asset::TexturePurpose purpose)
{
   switch (purpose) {
   case asset::TexturePurpose::Albedo:
      return ktx::TextureTranscode::BC1_RGB;
   case asset::TexturePurpose::AlbedoWithAlpha:
      return ktx::TextureTranscode::BC3_RGBA;
   case asset::TexturePurpose::NormalMap:
      return ktx::TextureTranscode::BC1_RGB;
   default:
      return ktx::TextureTranscode::BC4_R;
   }
}

}// namespace

bool import_texture(const TextureImportProps& props)
{
   fmt::print(stderr, "triglav-cli: Importing texture to {}\n", props.dstPath.string());

   int texWidth, texHeight, texChannels;
   stbi_uc* pixels = stbi_load(props.srcPath.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
   if (pixels == nullptr) {
      fmt::print(stderr, "triglav-cli: Source texture not found '{}'\n", props.srcPath.string());
      return EXIT_FAILURE;
   }

   const auto instance = GAPI_CHECK(graphics_api::Instance::create_instance());
   const auto device = GAPI_CHECK(instance.create_device(nullptr, graphics_api::DevicePickStrategy::PreferDedicated, 0));

   const auto texture = GAPI_CHECK(device->create_texture(
      format_from_purpose(props.purpose), {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)},
      graphics_api::TextureUsage::Sampled | graphics_api::TextureUsage::TransferDst | graphics_api::TextureUsage::TransferSrc,
      graphics_api::TextureState::Undefined, graphics_api::SampleCount::Single, props.hasMipMaps ? graphics_api::g_maxMipMaps : 1));

   GAPI_CHECK_STATUS(texture.write(*device, pixels));

   stbi_image_free(pixels);

   const auto ktxTexture = GAPI_CHECK(device->export_ktx_texture(texture));

   if (props.shouldCompress) {
      // TODO: Figure out how to transcode with isNormalMap = true.
      if (!ktxTexture.compress(ktx::TextureCompression::UASTC, false)) {
         return false;
      }

      if (!ktxTexture.transcode(transcode_from_purpose(props.purpose))) {
         return false;
      }
   }

   const auto outFile = io::open_file(props.dstPath, io::FileOpenMode::Create);
   if (!outFile.has_value()) {
      return false;
   }

   if (!asset::encode_texture(**outFile, props.purpose, ktxTexture, props.samplerProperties)) {
      return false;
   }

   return true;
}

}// namespace triglav::tool::cli
