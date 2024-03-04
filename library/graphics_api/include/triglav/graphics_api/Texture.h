#pragma once

#include "Buffer.h"
#include "vulkan/ObjectWrapper.hpp"

namespace triglav::graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(ImageView, Device)
DECLARE_VLK_WRAPPED_CHILD_OBJECT(Image, Device)

class CommandList;

class Texture
{
 public:
   Texture(vulkan::Image image, vulkan::DeviceMemory memory, vulkan::ImageView imageView,
           const ColorFormat &colorFormat, TextureType type, uint32_t width, uint32_t height, int mipCount);

   [[nodiscard]] VkImage vulkan_image() const;
   [[nodiscard]] VkImageView vulkan_image_view() const;
   [[nodiscard]] TextureType type() const;
   [[nodiscard]] uint32_t width() const;
   [[nodiscard]] uint32_t height() const;
   [[nodiscard]] Resolution resolution() const;
   Status write(Device &device, const uint8_t *pixels) const;
   [[nodiscard]] Status generate_mip_maps(const Device &device) const;

 private:
   void generate_mip_maps_internal(const CommandList &cmdList) const;

   uint32_t m_width{};
   uint32_t m_height{};
   ColorFormat m_colorFormat;
   TextureType m_type;
   vulkan::Image m_image;
   vulkan::DeviceMemory m_memory;
   vulkan::ImageView m_imageView;
   int m_mipCount;
};

}// namespace triglav::graphics_api