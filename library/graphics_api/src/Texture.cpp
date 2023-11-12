#include "Texture.h"

namespace graphics_api {

Texture::Texture(vulkan::Image image, vulkan::DeviceMemory memory, graphics_api::vulkan::ImageView imageView,
                 uint32_t width, uint32_t height) :
    m_image(std::move(image)),
    m_memory(std::move(memory)),
    m_imageView(std::move(imageView)),
    m_width{width},
    m_height{height}
{
}

VkImage Texture::vulkan_image() const
{
   return *m_image;
}

uint32_t Texture::width() const
{
   return m_width;
}

uint32_t Texture::height() const
{
   return m_height;
}

VkImageView Texture::vulkan_image_view() const
{
   return *m_imageView;
}

}// namespace graphics_api
