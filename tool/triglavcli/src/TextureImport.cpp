#include "TextureImport.hpp"

#include "ProjectConfig.hpp"

#include "triglav/graphics_api/Instance.hpp"
#include "triglav/io/File.hpp"

#include <format>
#include <iostream>
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

std::optional<ImageData> load_image_data(io::ISeekableStream& stream)
{
   struct StreamData
   {
      io::ISeekableStream& stream;
      bool isEOF;
   } streamData{
      .stream = stream,
      .isEOF = false,
   };

   stbi_io_callbacks callbacks;
   callbacks.read = [](void* user, char* data, const int size) -> int {
      auto* stream = static_cast<StreamData*>(user);
      const auto res = stream->stream.read({reinterpret_cast<u8*>(data), static_cast<MemorySize>(size)});
      if (!res.has_value() || *res != static_cast<MemorySize>(size)) {
         stream->isEOF = true;
         return static_cast<int>(*res);
      }
      return static_cast<int>(*res);
   };
   callbacks.skip = [](void* user, const int num) {
      auto* stream = static_cast<StreamData*>(user);
      [[maybe_unused]] const auto res = stream->stream.seek(io::SeekPosition::Current, num);
      assert(res == io::Status::Success);
   };
   callbacks.eof = [](void* user) -> int {
      auto* stream = static_cast<StreamData*>(user);
      return stream->isEOF ? 0 : 1;
   };

   int texWidth, texHeight, texChannels;
   stbi_uc* pixels = stbi_load_from_callbacks(&callbacks, &streamData, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
   if (pixels == nullptr) {
      std::print(std::cerr, "triglav-cli: Failed to load image from memory\n");
      return std::nullopt;
   }

   ImageData outData{};
   outData.imageData.resize(sizeof(u32) * texWidth * texHeight);
   std::memcpy(outData.imageData.data(), pixels, sizeof(u32) * texWidth * texHeight);
   outData.size = {texWidth, texHeight};

   stbi_image_free(pixels);

   return outData;
}

bool import_texture_from_stream(const TextureImportProps& props, io::ISeekableStream& stream)
{
   if (!props.shouldOverride && props.dstPath.exists()) {
      std::print(std::cerr, "triglav-cli: Failed to import texture to {}, file exists", props.dstPath.string());
      return false;
   }

   std::print(std::cerr, "triglav-cli: Importing texture to {}\n", props.dstPath.string());

   auto data = load_image_data(stream);
   if (!data.has_value()) {
      return false;
   }

   const auto instance = GAPI_CHECK(graphics_api::Instance::create_instance(nullptr));
   const auto device = GAPI_CHECK(instance.create_device(nullptr, graphics_api::DevicePickStrategy::PreferDedicated, 0));

   const auto texture = GAPI_CHECK(device->create_texture(
      format_from_purpose(props.purpose), {data->size.x, data->size.y},
      graphics_api::TextureUsage::Sampled | graphics_api::TextureUsage::TransferDst | graphics_api::TextureUsage::TransferSrc,
      graphics_api::TextureState::Undefined, graphics_api::SampleCount::Single, props.hasMipMaps ? graphics_api::g_maxMipMaps : 1));

   GAPI_CHECK_STATUS(texture.write(*device, reinterpret_cast<const u8*>(data->imageData.data())));

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

bool import_texture(const TextureImportProps& props)
{
   auto fileHandle = io::open_file(props.srcPath, io::FileOpenMode::Read);
   if (!fileHandle.has_value()) {
      return false;
   }

   return import_texture_from_stream(props, **fileHandle);
}

}// namespace triglav::tool::cli
