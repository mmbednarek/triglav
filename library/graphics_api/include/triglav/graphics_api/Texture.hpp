#pragma once

#include "Buffer.hpp"
#include "TextureView.hpp"
#include "vulkan/ObjectWrapper.hpp"

namespace triglav::graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(Image, Device)

class CommandList;

class Texture
{
 public:
   Texture(vulkan::Image image, vulkan::DeviceMemory memory, vulkan::ImageView imageView, const ColorFormat& colorFormat,
           TextureUsageFlags usageFlags, uint32_t width, uint32_t height, int mipCount);

   [[nodiscard]] VkImage vulkan_image() const;
   [[nodiscard]] VkImageView vulkan_image_view() const;
   [[nodiscard]] TextureUsageFlags usage_flags() const;
   [[nodiscard]] uint32_t width() const;
   [[nodiscard]] uint32_t height() const;
   [[nodiscard]] Resolution resolution() const;
   [[nodiscard]] const SamplerProperties& sampler_properties() const;
   Status write(Device& device, const uint8_t* pixels) const;
   [[nodiscard]] Status generate_mip_maps(Device& device) const;
   [[nodiscard]] Result<TextureView> create_mip_view(const Device& device, u32 mipLevel) const;

   void set_anisotropy_state(bool isEnabled);
   void set_lod(float min, float max);
   void set_debug_name(std::string_view name);

 private:
   void generate_mip_maps_internal(const CommandList& cmdList) const;

   uint32_t m_width{};
   uint32_t m_height{};
   ColorFormat m_colorFormat;
   TextureUsageFlags m_usageFlags;
   vulkan::Image m_image;
   vulkan::DeviceMemory m_memory;
   vulkan::ImageView m_imageView;
   int m_mipCount;
   SamplerProperties m_samplerProperties;
};

}// namespace triglav::graphics_api