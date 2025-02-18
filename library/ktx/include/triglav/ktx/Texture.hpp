#pragma once

extern "C"
{
#include "ForwardDecl.h"
}
#include "triglav/io/Path.hpp"

#include <optional>

namespace triglav::ktx {

class VulkanDeviceInfo;

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

   static std::optional<Texture> from_file(const io::Path& path);

 private:
   ::ktxTexture* m_ktxTexture;
};

}// namespace triglav::ktx