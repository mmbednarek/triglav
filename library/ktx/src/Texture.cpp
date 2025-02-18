#include "Texture.hpp"

#include <utility>
extern "C"
{
#include <vulkan/vulkan.h>

#include <ktx.h>
#include <ktxvulkan.h>
}

namespace triglav::ktx {

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

std::optional<Texture> Texture::from_file(const io::Path& path)
{
   ::ktxTexture* ktxTexture;
   const auto result = ktxTexture_CreateFromNamedFile(path.string().c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
   if (result != KTX_SUCCESS) {
      return std::nullopt;
   }
   return Texture{ktxTexture};
}

}// namespace triglav::asset