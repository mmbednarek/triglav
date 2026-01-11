#include "TextureImport.hpp"

#include "triglav/project/Project.hpp"

#include "triglav/graphics_api/Instance.hpp"
#include "triglav/io/File.hpp"

#include <format>
#include <iostream>
#include <stbi/stb_image.h>

namespace triglav::tool::cli {

namespace {

graphics_api::ColorFormat format_from_purpose(const asset::TexturePurpose purpose, const bool extract_channel)
{
   if (purpose == asset::TexturePurpose::Albedo || purpose == asset::TexturePurpose::AlbedoWithAlpha) {
      return extract_channel ? GAPI_FORMAT(R, sRGB) : GAPI_FORMAT(RGBA, sRGB);
   }
   return extract_channel ? GAPI_FORMAT(R, UNorm8) : GAPI_FORMAT(RGBA, UNorm8);
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

std::vector<u8> extract_channel(const ImageData& data, TextureChannel channel)
{
   std::vector<u8> result(data.size.x * data.size.y);
   for (u32 y = 0; y < data.size.y; ++y) {
      for (u32 x = 0; x < data.size.x; ++x) {
         const u32 offset = 8 * static_cast<u32>(channel);
         const u32 pixel = data.image_data[x + y * data.size.x];
         result[x + y * data.size.x] = (pixel >> offset) & 0xFF;
      }
   }
   return result;
}

}// namespace

std::optional<ImageData> load_image_data(io::ISeekableStream& stream)
{
   struct StreamData
   {
      io::ISeekableStream& stream;
      bool is_eof;
   } stream_data{
      .stream = stream,
      .is_eof = false,
   };

   stbi_io_callbacks callbacks;
   callbacks.read = [](void* user, char* data, const int size) -> int {
      auto* stream = static_cast<StreamData*>(user);
      const auto res = stream->stream.read({reinterpret_cast<u8*>(data), static_cast<MemorySize>(size)});
      if (!res.has_value() || *res != static_cast<MemorySize>(size)) {
         stream->is_eof = true;
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
      return stream->is_eof ? 0 : 1;
   };

   int tex_width, tex_height, tex_channels;
   stbi_uc* pixels = stbi_load_from_callbacks(&callbacks, &stream_data, &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
   if (pixels == nullptr) {
      std::print(std::cerr, "triglav-cli: Failed to load image from memory\n");
      return std::nullopt;
   }

   ImageData out_data{};
   out_data.image_data.resize(sizeof(u32) * tex_width * tex_height);
   std::memcpy(out_data.image_data.data(), pixels, sizeof(u32) * tex_width * tex_height);
   out_data.size = {tex_width, tex_height};

   stbi_image_free(pixels);

   return out_data;
}

bool import_texture_from_stream(const TextureImportProps& props, io::ISeekableStream& stream)
{
   if (!props.should_override && props.dst_path.exists()) {
      std::print(std::cerr, "triglav-cli: Failed to import texture to {}, file exists", props.dst_path.string());
      return false;
   }

   std::print(std::cerr, "triglav-cli: Importing texture to {}\n", props.dst_path.string());

   auto data = load_image_data(stream);
   if (!data.has_value()) {
      return false;
   }

   const auto instance = GAPI_CHECK(graphics_api::Instance::create_instance(nullptr));
   const auto device = GAPI_CHECK(instance.create_device(nullptr, graphics_api::DevicePickStrategy::PreferDedicated, 0));

   const auto texture = GAPI_CHECK(device->create_texture(
      format_from_purpose(props.purpose, props.extract_channel.has_value()), {data->size.x, data->size.y},
      graphics_api::TextureUsage::Sampled | graphics_api::TextureUsage::TransferDst | graphics_api::TextureUsage::TransferSrc,
      graphics_api::TextureState::Undefined, graphics_api::SampleCount::Single, props.has_mip_maps ? graphics_api::g_max_mip_maps : 1));

   if (props.extract_channel.has_value()) {
      const auto channel_data = extract_channel(*data, *props.extract_channel);
      GAPI_CHECK_STATUS(texture.write(*device, channel_data.data()));
   } else {
      GAPI_CHECK_STATUS(texture.write(*device, reinterpret_cast<const u8*>(data->image_data.data())));
   }

   const auto ktx_texture = GAPI_CHECK(device->export_ktx_texture(texture));

   if (props.should_compress) {
      // TODO: Figure out how to transcode with is_normal_map = true.
      if (!ktx_texture.compress(ktx::TextureCompression::UASTC, false)) {
         return false;
      }

      if (!ktx_texture.transcode(transcode_from_purpose(props.purpose))) {
         return false;
      }
   }

   const auto out_file = io::open_file(props.dst_path, io::FileMode::Write | io::FileMode::Create);
   if (!out_file.has_value()) {
      return false;
   }

   if (!asset::encode_texture(**out_file, props.purpose, ktx_texture, props.sampler_properties)) {
      return false;
   }

   return true;
}

bool import_texture(const TextureImportProps& props)
{
   auto file_handle = io::open_file(props.src_path, io::FileMode::Read);
   if (!file_handle.has_value()) {
      return false;
   }

   return import_texture_from_stream(props, **file_handle);
}

}// namespace triglav::tool::cli
