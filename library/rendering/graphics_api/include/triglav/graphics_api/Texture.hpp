#pragma once

#include "Buffer.hpp"
#include "TextureView.hpp"
#include "vulkan/ObjectWrapper.hpp"

#include <optional>
#include <variant>

namespace triglav::graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(Image, Device)

class CommandList;

class Texture
{
 public:
   Texture(vulkan::Image image, vulkan::DeviceMemory memory, vulkan::ImageView image_view, const ColorFormat& color_format,
           TextureUsageFlags usage_flags, uint32_t width, uint32_t height, int mip_count);

   Texture(VkImage image, vulkan::ImageView image_view, const ColorFormat& color_format, TextureUsageFlags usage_flags, uint32_t width,
           uint32_t height, int mip_count);

   [[nodiscard]] VkImage vulkan_image() const;
   [[nodiscard]] VkImageView vulkan_image_view() const;
   [[nodiscard]] TextureUsageFlags usage_flags() const;
   [[nodiscard]] uint32_t width() const;
   [[nodiscard]] uint32_t height() const;
   [[nodiscard]] Resolution resolution() const;
   [[nodiscard]] ColorFormat format() const;
   [[nodiscard]] SamplerProperties& sampler_properties();
   [[nodiscard]] const SamplerProperties& sampler_properties() const;
   Status write(Device& device, const uint8_t* pixels) const;
   [[nodiscard]] Status generate_mip_maps(Device& device) const;
   [[nodiscard]] Result<TextureView> create_mip_view(const Device& device, u32 mip_level) const;
   [[nodiscard]] u32 mip_count() const;
   [[nodiscard]] TextureView& view();
   [[nodiscard]] const TextureView& view() const;

   void set_anisotropy_state(bool is_enabled);
   void set_lod(float min, float max);
   void set_debug_name(std::string_view name);

 private:
   void generate_mip_maps_internal(const CommandList& cmd_list) const;

   uint32_t m_width{};
   uint32_t m_height{};
   ColorFormat m_color_format;
   TextureUsageFlags m_usage_flags;
   std::variant<vulkan::Image, VkImage> m_image;// owning or non-owning
   std::optional<vulkan::DeviceMemory> m_memory;
   TextureView m_texture_view;
   int m_mip_count;
   SamplerProperties m_sampler_properties;
};

}// namespace triglav::graphics_api