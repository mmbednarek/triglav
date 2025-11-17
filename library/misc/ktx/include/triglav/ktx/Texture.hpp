#pragma once

extern "C"
{
#include "ForwardDecl.h"
}
#include "triglav/Math.hpp"
#include "triglav/io/Path.hpp"
#include "triglav/io/Stream.hpp"

#include <optional>

namespace triglav::ktx {

class VulkanDeviceInfo;

enum class Format
{
   R8G8B8A8_SRGB,
   R8G8B8A8_UNORM,
};

enum class TextureCompression
{
   ETC,
   UASTC,
   ZLIB,
};

struct TextureCreateInfo
{
   Format format;
   Vector2u dimensions;
   bool generate_mipmaps;
   bool create_mip_layers;
};

enum class TextureTranscode
{
   BC1_RGB,
   BC3_RGBA,
   BC4_R,
};

class Texture
{
   friend class VulkanDeviceInfo;

 public:
   explicit Texture(::ktxTexture* ktxTexture);
   ~Texture();

   Texture(const Texture& other) = delete;
   Texture& operator=(const Texture& other) = delete;

   Texture(Texture&& other) noexcept;
   Texture& operator=(Texture&& other) noexcept;

   [[nodiscard]] bool set_image_from_buffer(std::span<const u8> buffer, u32 mip_level, u32 layer, u32 face_slice) const;
   [[nodiscard]] bool write_to_stream(io::IWriter& writer) const;
   [[nodiscard]] bool write_to_file(const io::Path& path) const;
   [[nodiscard]] bool compress(TextureCompression compression, bool is_normal_map) const;
   [[nodiscard]] bool transcode(TextureTranscode transcode) const;
   [[nodiscard]] bool uncompress() const;
   [[nodiscard]] bool is_compressed() const;
   [[nodiscard]] Vector2u dimensions() const;
   void print_debug_info() const;
   void generate_mipmaps() const;

   static std::optional<Texture> create(const TextureCreateInfo& info);
   static std::optional<Texture> from_file(const io::Path& path);
   static std::optional<Texture> from_stream(io::ISeekableStream& seekable_stream);

 private:
   ::ktxTexture* m_ktxTexture;
};

}// namespace triglav::ktx