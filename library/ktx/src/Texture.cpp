#include "Texture.hpp"

#include <utility>
extern "C"
{
#include <vulkan/vulkan.h>

#include <cassert>
#include <ktx.h>
#include <ktxvulkan.h>
}
#include <fmt/core.h>

namespace triglav::ktx {

namespace {

ktxStream create_ktx_stream_from_io_stream(io::ISeekableStream& seekableStream)
{
   ktxStream resultStream{};
   resultStream.data.custom_ptr.address = &seekableStream;
   resultStream.type = eStreamTypeCustom;

   resultStream.read = [](ktxStream* str, void* dst, const ktx_size_t count) -> KTX_error_code {
      auto* userData = static_cast<io::ISeekableStream*>(str->data.custom_ptr.address);
      if (!userData->read({static_cast<u8*>(dst), count}).has_value()) {
         return KTX_FILE_READ_ERROR;
      }
      return KTX_SUCCESS;
   };

   resultStream.skip = [](ktxStream* str, const ktx_size_t count) -> KTX_error_code {
      auto* userData = static_cast<io::ISeekableStream*>(str->data.custom_ptr.address);
      if (userData->seek(io::SeekPosition::Current, static_cast<MemoryOffset>(count)) != io::Status::Success) {
         return KTX_FILE_READ_ERROR;
      }
      return KTX_SUCCESS;
   };

   resultStream.write = [](ktxStream* str, const void* src, const ktx_size_t size, const ktx_size_t count) -> KTX_error_code {
      auto* userData = static_cast<io::ISeekableStream*>(str->data.custom_ptr.address);
      if (!userData->write({static_cast<const u8*>(src), size * count}).has_value()) {
         return KTX_FILE_WRITE_ERROR;
      }
      return KTX_SUCCESS;
   };

   resultStream.getpos = [](ktxStream* str, ktx_off_t* const offset) -> KTX_error_code {
      const auto* userData = static_cast<io::ISeekableStream*>(str->data.custom_ptr.address);
      *offset = static_cast<ktx_off_t>(userData->position());
      return KTX_SUCCESS;
   };

   resultStream.setpos = [](ktxStream* str, const ktx_off_t offset) -> KTX_error_code {
      auto* userData = static_cast<io::ISeekableStream*>(str->data.custom_ptr.address);
      if (userData->seek(io::SeekPosition::Begin, offset) != io::Status::Success) {
         return KTX_FILE_READ_ERROR;
      }
      return KTX_SUCCESS;
   };

   resultStream.getsize = [](ktxStream* str, ktx_size_t* const size) -> KTX_error_code {
      auto* userData = static_cast<io::ISeekableStream*>(str->data.custom_ptr.address);
      const auto currPos = userData->position();
      if (userData->seek(io::SeekPosition::End, 0) != io::Status::Success) {
         return KTX_FILE_READ_ERROR;
      }
      *size = userData->position();
      if (userData->seek(io::SeekPosition::Begin, static_cast<MemoryOffset>(currPos)) != io::Status::Success) {
         return KTX_FILE_READ_ERROR;
      }
      return KTX_SUCCESS;
   };

   resultStream.destruct = [](ktxStream* /*str*/) -> void {};
   resultStream.readpos = 0;
   resultStream.closeOnDestruct = false;

   return resultStream;
}

struct WriterStreamUserData
{
   io::IWriter& writer;
   MemorySize position;
};

std::tuple<ktxStream, std::unique_ptr<WriterStreamUserData>> create_ktx_stream_from_io_writer(io::IWriter& writer)
{
   auto userData = std::make_unique<WriterStreamUserData>(writer, 0);

   ktxStream resultStream{};
   resultStream.data.custom_ptr.address = userData.get();
   resultStream.type = eStreamTypeCustom;

   resultStream.read = [](ktxStream* /*str*/, void* /*dst*/, const ktx_size_t /*count*/) -> KTX_error_code {
      assert(0);
      return KTX_FILE_DATA_ERROR;
   };

   resultStream.skip = [](ktxStream* /*str*/, const ktx_size_t /*count*/) -> KTX_error_code {
      assert(0);
      return KTX_FILE_DATA_ERROR;
   };

   resultStream.write = [](ktxStream* str, const void* src, const ktx_size_t size, const ktx_size_t count) -> KTX_error_code {
      auto* userData = static_cast<WriterStreamUserData*>(str->data.custom_ptr.address);
      if (!userData->writer.write({static_cast<const u8*>(src), size * count}).has_value()) {
         return KTX_FILE_WRITE_ERROR;
      }
      userData->position += size * count;
      return KTX_SUCCESS;
   };

   resultStream.getpos = [](ktxStream* str, ktx_off_t* const offset) -> KTX_error_code {
      auto* userData = static_cast<WriterStreamUserData*>(str->data.custom_ptr.address);
      *offset = userData->position;
      return KTX_SUCCESS;
   };

   resultStream.setpos = [](ktxStream* /*str*/, const ktx_off_t /*offset*/) -> KTX_error_code {
      assert(0);
      return KTX_FILE_DATA_ERROR;
   };

   resultStream.getsize = [](ktxStream* /*str*/, ktx_size_t* const /*size*/) -> KTX_error_code {
      assert(0);
      return KTX_FILE_DATA_ERROR;
   };

   resultStream.destruct = [](ktxStream* /*str*/) -> void {};
   resultStream.readpos = 0;
   resultStream.closeOnDestruct = false;

   return {resultStream, std::move(userData)};
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

bool Texture::set_image_from_buffer(const std::span<const u8> buffer, const u32 mipLevel, const u32 layer, const u32 faceSlice) const
{
   const auto result = ktxTexture_SetImageFromMemory(m_ktxTexture, mipLevel, layer, faceSlice, buffer.data(), buffer.size());
   return result == KTX_SUCCESS;
}

bool Texture::write_to_stream(io::IWriter& writer) const
{
   auto [outStream, userData] = create_ktx_stream_from_io_writer(writer);
   ktxTexture_WriteToNamedFile(m_ktxTexture, "debug.ktx");
   return ktxTexture_WriteToStream(m_ktxTexture, &outStream) == KTX_SUCCESS;
}

bool Texture::write_to_file(const io::Path& path) const
{
   return ktxTexture_WriteToNamedFile(m_ktxTexture, path.string().c_str()) == KTX_SUCCESS;
}

bool Texture::compress(const TextureCompression compression, const bool isNormalMap) const
{
   if (compression == TextureCompression::ZLIB) {
      return ktxTexture2_DeflateZLIB(reinterpret_cast<ktxTexture2*>(m_ktxTexture), 5) == KTX_SUCCESS;
   }

   ktxBasisParams params{};
   params.structSize = sizeof(ktxBasisParams);
   params.normalMap = isNormalMap ? KTX_TRUE : KTX_FALSE;
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

   ktx_transcode_fmt_e transcodeFormat;
   switch (transcode) {
   case TextureTranscode::BC1_RGB:
      transcodeFormat = KTX_TTF_BC1_RGB;
      break;
   case TextureTranscode::BC3_RGBA:
      transcodeFormat = KTX_TTF_BC3_RGBA;
      break;
   case TextureTranscode::BC4_R:
      transcodeFormat = KTX_TTF_BC4_R;
      break;
   default:
      return false;
   }

   return ktxTexture2_TranscodeBasis(reinterpret_cast<ktxTexture2*>(m_ktxTexture), transcodeFormat, 0) == KTX_SUCCESS;
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
   auto iterator = [](const int mipLevel, const int /*face*/, const int width, const int height, const int /*depth*/,
                      const ktx_uint64_t /*faceLodSize*/, void* /*pixels*/, void* userdata) -> KTX_error_code {
      if (mipLevel == 0) {
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
   fmt::print(stderr, "Format: {}\n", static_cast<u32>(ktxTexture2_GetVkFormat(reinterpret_cast<ktxTexture2*>(m_ktxTexture))));
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
      info.createMipLayers ? static_cast<int>(std::floor(std::log2(std::max(info.dimensions.x, info.dimensions.y)))) + 1 : 1;
   ktxInfo.numLayers = 1;
   ktxInfo.numFaces = 1;
   ktxInfo.isArray = KTX_FALSE;
   ktxInfo.generateMipmaps = info.generateMipmaps ? KTX_TRUE : KTX_FALSE;

   ::ktxTexture2* resultTexture{};
   if (ktxTexture2_Create(&ktxInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &resultTexture) != KTX_SUCCESS) {
      return std::nullopt;
   }

   return Texture{ktxTexture(resultTexture)};
}

std::optional<Texture> Texture::from_file(const io::Path& path)
{
   ::ktxTexture* ktxTexture;
   if (ktxTexture_CreateFromNamedFile(path.string().c_str(), KTX_TEXTURE_CREATE_ALLOC_STORAGE, &ktxTexture) != KTX_SUCCESS) {
      return std::nullopt;
   }
   return Texture{ktxTexture};
}

std::optional<Texture> Texture::from_stream(io::ISeekableStream& seekableStream)
{
   auto inputKtxStream = create_ktx_stream_from_io_stream(seekableStream);

   ::ktxTexture* ktxTexture;
   if (ktxTexture_CreateFromStream(&inputKtxStream, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture) != KTX_SUCCESS) {
      return std::nullopt;
   }

   return Texture{ktxTexture};
}

}// namespace triglav::ktx