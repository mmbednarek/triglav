#include "Texture.hpp"

#include <cstdlib>
#include <utility>
extern "C"
{
#include <vulkan/vulkan.h>

#include <cassert>
#include <ktx.h>
#include <ktxvulkan.h>
}
#include <memory>
#include <print>

namespace triglav::ktx {

namespace {

ktxStream create_ktx_stream_from_io_stream(io::ISeekableStream& seekable_stream)
{
   ktxStream result_stream{};
   result_stream.data.custom_ptr.address = &seekable_stream;
   result_stream.type = eStreamTypeCustom;

   result_stream.read = [](ktxStream* str, void* dst, const ktx_size_t count) -> KTX_error_code {
      auto* user_data = static_cast<io::ISeekableStream*>(str->data.custom_ptr.address);
      if (!user_data->read({static_cast<u8*>(dst), count}).has_value()) {
         return KTX_FILE_READ_ERROR;
      }
      return KTX_SUCCESS;
   };

   result_stream.skip = [](ktxStream* str, const ktx_size_t count) -> KTX_error_code {
      auto* user_data = static_cast<io::ISeekableStream*>(str->data.custom_ptr.address);
      if (user_data->seek(io::SeekPosition::Current, static_cast<MemoryOffset>(count)) != io::Status::Success) {
         return KTX_FILE_READ_ERROR;
      }
      return KTX_SUCCESS;
   };

   result_stream.write = [](ktxStream* str, const void* src, const ktx_size_t size, const ktx_size_t count) -> KTX_error_code {
      auto* user_data = static_cast<io::ISeekableStream*>(str->data.custom_ptr.address);
      if (!user_data->write({static_cast<const u8*>(src), size * count}).has_value()) {
         return KTX_FILE_WRITE_ERROR;
      }
      return KTX_SUCCESS;
   };

   result_stream.getpos = [](ktxStream* str, ktx_off_t* const offset) -> KTX_error_code {
      const auto* user_data = static_cast<io::ISeekableStream*>(str->data.custom_ptr.address);
      *offset = static_cast<ktx_off_t>(user_data->position());
      return KTX_SUCCESS;
   };

   result_stream.setpos = [](ktxStream* str, const ktx_off_t offset) -> KTX_error_code {
      auto* user_data = static_cast<io::ISeekableStream*>(str->data.custom_ptr.address);
      if (user_data->seek(io::SeekPosition::Begin, offset) != io::Status::Success) {
         return KTX_FILE_READ_ERROR;
      }
      return KTX_SUCCESS;
   };

   result_stream.getsize = [](ktxStream* str, ktx_size_t* const size) -> KTX_error_code {
      auto* user_data = static_cast<io::ISeekableStream*>(str->data.custom_ptr.address);
      const auto curr_pos = user_data->position();
      if (user_data->seek(io::SeekPosition::End, 0) != io::Status::Success) {
         return KTX_FILE_READ_ERROR;
      }
      *size = user_data->position();
      if (user_data->seek(io::SeekPosition::Begin, static_cast<MemoryOffset>(curr_pos)) != io::Status::Success) {
         return KTX_FILE_READ_ERROR;
      }
      return KTX_SUCCESS;
   };

   result_stream.destruct = [](ktxStream* /*str*/) -> void {};
   result_stream.readpos = 0;
   result_stream.closeOnDestruct = false;

   return result_stream;
}

struct WriterStreamUserData
{
   io::IWriter& writer;
   MemorySize position;
};

std::tuple<ktxStream, std::unique_ptr<WriterStreamUserData>> create_ktx_stream_from_io_writer(io::IWriter& writer)
{
   auto user_data = std::make_unique<WriterStreamUserData>(writer, 0);

   ktxStream result_stream{};
   result_stream.data.custom_ptr.address = user_data.get();
   result_stream.type = eStreamTypeCustom;

   result_stream.read = [](ktxStream* /*str*/, void* /*dst*/, const ktx_size_t /*count*/) -> KTX_error_code {
      assert(0);
      return KTX_FILE_DATA_ERROR;
   };

   result_stream.skip = [](ktxStream* /*str*/, const ktx_size_t /*count*/) -> KTX_error_code {
      assert(0);
      return KTX_FILE_DATA_ERROR;
   };

   result_stream.write = [](ktxStream* str, const void* src, const ktx_size_t size, const ktx_size_t count) -> KTX_error_code {
      auto* user_data = static_cast<WriterStreamUserData*>(str->data.custom_ptr.address);
      if (!user_data->writer.write({static_cast<const u8*>(src), size * count}).has_value()) {
         return KTX_FILE_WRITE_ERROR;
      }
      user_data->position += size * count;
      return KTX_SUCCESS;
   };

   result_stream.getpos = [](ktxStream* str, ktx_off_t* const offset) -> KTX_error_code {
      auto* user_data = static_cast<WriterStreamUserData*>(str->data.custom_ptr.address);
      *offset = user_data->position;
      return KTX_SUCCESS;
   };

   result_stream.setpos = [](ktxStream* /*str*/, const ktx_off_t /*offset*/) -> KTX_error_code {
      assert(0);
      return KTX_FILE_DATA_ERROR;
   };

   result_stream.getsize = [](ktxStream* /*str*/, ktx_size_t* const /*size*/) -> KTX_error_code {
      assert(0);
      return KTX_FILE_DATA_ERROR;
   };

   result_stream.destruct = [](ktxStream* /*str*/) -> void {};
   result_stream.readpos = 0;
   result_stream.closeOnDestruct = false;

   return {result_stream, std::move(user_data)};
}

}// namespace

Texture::Texture(ktxTexture* ktxTexture) :
    m_ktxTexture(ktxTexture)
{
}

Texture::~Texture()
{
   if (m_ktxTexture != nullptr) {
      ktxTexture_Destroy(m_ktxTexture);
   }
}

Texture::Texture(Texture&& other) noexcept :
    m_ktxTexture(std::exchange(other.m_ktxTexture, nullptr))
{
}

Texture& Texture::operator=(Texture&& other) noexcept
{
   m_ktxTexture = std::exchange(other.m_ktxTexture, nullptr);
   return *this;
}

bool Texture::set_image_from_buffer(const std::span<const u8> buffer, const u32 mip_level, const u32 layer, const u32 face_slice) const
{
   const auto result = ktxTexture_SetImageFromMemory(m_ktxTexture, mip_level, layer, face_slice, buffer.data(), buffer.size());
   return result == KTX_SUCCESS;
}

bool Texture::write_to_stream(io::IWriter& writer) const
{
   auto [out_stream, user_data] = create_ktx_stream_from_io_writer(writer);
   ktxTexture_WriteToNamedFile(m_ktxTexture, "debug.ktx");
   return ktxTexture_WriteToStream(m_ktxTexture, &out_stream) == KTX_SUCCESS;
}

bool Texture::write_to_file(const io::Path& path) const
{
   return ktxTexture_WriteToNamedFile(m_ktxTexture, path.string().c_str()) == KTX_SUCCESS;
}

bool Texture::compress(const TextureCompression compression, const bool is_normal_map) const
{
   if (compression == TextureCompression::ZLIB) {
      return ktxTexture2_DeflateZLIB(reinterpret_cast<ktxTexture2*>(m_ktxTexture), 5) == KTX_SUCCESS;
   }

   ktxBasisParams params{};
   params.structSize = sizeof(ktxBasisParams);
   params.normalMap = is_normal_map ? KTX_TRUE : KTX_FALSE;
   switch (compression) {
   case TextureCompression::ETC:
      params.compressionLevel = KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL;
      break;
   case TextureCompression::UASTC:
      params.uastc = KTX_TRUE;
      break;
   default:
      return false;
   }


   return ktxTexture2_CompressBasisEx(reinterpret_cast<ktxTexture2*>(m_ktxTexture), &params) == KTX_SUCCESS;
}

bool Texture::transcode(const TextureTranscode transcode) const
{
   if (!ktxTexture2_NeedsTranscoding(reinterpret_cast<ktxTexture2*>(m_ktxTexture))) {
      return true;
   }

   ktx_transcode_fmt_e transcode_format;
   switch (transcode) {
   case TextureTranscode::BC1_RGB:
      transcode_format = KTX_TTF_BC1_RGB;
      break;
   case TextureTranscode::BC3_RGBA:
      transcode_format = KTX_TTF_BC3_RGBA;
      break;
   case TextureTranscode::BC4_R:
      transcode_format = KTX_TTF_BC4_R;
      break;
   default:
      return false;
   }

   return ktxTexture2_TranscodeBasis(reinterpret_cast<ktxTexture2*>(m_ktxTexture), transcode_format, 0) == KTX_SUCCESS;
}

bool Texture::uncompress() const
{
   return ktxTexture2_TranscodeBasis(reinterpret_cast<ktxTexture2*>(m_ktxTexture), KTX_TTF_RGBA32, 0) == KTX_SUCCESS;
}

bool Texture::is_compressed() const
{
   return m_ktxTexture->isCompressed;
}

Vector2u Texture::dimensions() const
{
   Vector2u result{};
   auto iterator = [](const int mip_level, const int /*face*/, const int width, const int height, const int /*depth*/,
                      const ktx_uint64_t /*face_lod_size*/, void* /*pixels*/, void* userdata) -> KTX_error_code {
      if (mip_level == 0) {
         auto* res = static_cast<Vector2u*>(userdata);
         res->x = width;
         res->y = height;
      }
      return KTX_SUCCESS;
   };

   ktxTexture_IterateLevels(m_ktxTexture, iterator, &result);
   return result;
}

void Texture::print_debug_info() const
{
   std::println(stderr, "Format: {}", static_cast<u32>(ktxTexture2_GetVkFormat(reinterpret_cast<ktxTexture2*>(m_ktxTexture))));
}

void Texture::generate_mipmaps() const {}

std::optional<Texture> Texture::create(const TextureCreateInfo& info)
{
   ktxTextureCreateInfo ktxInfo{};
   switch (info.format) {
   case Format::R8G8B8A8_SRGB:
      ktxInfo.vkFormat = VK_FORMAT_R8G8B8A8_SRGB;
      break;
   case Format::R8G8B8A8_UNORM:
      ktxInfo.vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
      break;
   }
   ktxInfo.baseWidth = info.dimensions.x;
   ktxInfo.baseHeight = info.dimensions.y;
   ktxInfo.baseDepth = 1;
   ktxInfo.numDimensions = 2;
   ktxInfo.numLevels =
      info.create_mip_layers ? static_cast<int>(std::floor(std::log2(std::max(info.dimensions.x, info.dimensions.y)))) + 1 : 1;
   ktxInfo.numLayers = 1;
   ktxInfo.numFaces = 1;
   ktxInfo.isArray = KTX_FALSE;
   ktxInfo.generateMipmaps = info.generate_mipmaps ? KTX_TRUE : KTX_FALSE;

   ::ktxTexture2* result_texture{};
   if (ktxTexture2_Create(&ktxInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &result_texture) != KTX_SUCCESS) {
      return std::nullopt;
   }

   return Texture{ktxTexture(result_texture)};
}

std::optional<Texture> Texture::from_file(const io::Path& path)
{
   ::ktxTexture* ktxTexture;
   if (ktxTexture_CreateFromNamedFile(path.string().c_str(), KTX_TEXTURE_CREATE_ALLOC_STORAGE, &ktxTexture) != KTX_SUCCESS) {
      return std::nullopt;
   }
   return Texture{ktxTexture};
}

std::optional<Texture> Texture::from_stream(io::ISeekableStream& seekable_stream)
{
   auto input_ktx_stream = create_ktx_stream_from_io_stream(seekable_stream);

   ::ktxTexture* ktxTexture;
   if (ktxTexture_CreateFromStream(&input_ktx_stream, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture) != KTX_SUCCESS) {
      return std::nullopt;
   }

   return Texture{ktxTexture};
}

}// namespace triglav::ktx
