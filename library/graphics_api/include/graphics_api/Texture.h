#pragma once

#include "Buffer.h"
#include "vulkan/ObjectWrapper.hpp"

namespace graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(ImageView, Device)
DECLARE_VLK_WRAPPED_CHILD_OBJECT(Image, Device)

class Texture
{
 public:
   Texture(vulkan::Image image, vulkan::DeviceMemory memory, vulkan::ImageView imageView,
           const ColorFormat &colorFormat, uint32_t width, uint32_t height);

   [[nodiscard]] VkImage vulkan_image() const;
   [[nodiscard]] VkImageView vulkan_image_view() const;
   [[nodiscard]] uint32_t width() const;
   [[nodiscard]] uint32_t height() const;
   Status write(Device &device, const uint8_t *pixels) const;

 private:
   uint32_t m_width{};
   uint32_t m_height{};
   ColorFormat m_colorFormat;
   vulkan::Image m_image;
   vulkan::DeviceMemory m_memory;
   vulkan::ImageView m_imageView;
};

}// namespace graphics_api