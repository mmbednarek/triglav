#include "Texture.hpp"

#include "CommandList.hpp"
#include "Device.hpp"
#include "vulkan/Util.hpp"

namespace triglav::graphics_api {

Texture::Texture(vulkan::Image image, vulkan::DeviceMemory memory, vulkan::ImageView image_view, const ColorFormat& color_format,
                 const TextureUsageFlags usage_flags, const uint32_t width, const uint32_t height, const int mip_count) :
    m_width{width},
    m_height{height},
    m_color_format(color_format),
    m_usage_flags(usage_flags),
    m_image(std::move(image)),
    m_memory(std::move(memory)),
    m_texture_view(std::move(image_view), usage_flags),
    m_mip_count{mip_count},
    m_sampler_properties{
       FilterType::Linear,
       FilterType::Linear,
       TextureAddressMode::Repeat,
       TextureAddressMode::Repeat,
       TextureAddressMode::Repeat,
       true,
       0.0f,
       0.0f,
       static_cast<float>(mip_count),
    }
{
}

Texture::Texture(VkImage image, vulkan::ImageView image_view, const ColorFormat& color_format, TextureUsageFlags usage_flags,
                 const u32 width, const u32 height, const int mip_count) :
    m_width{width},
    m_height{height},
    m_color_format(color_format),
    m_usage_flags(usage_flags),
    m_image(image),
    m_texture_view(std::move(image_view), usage_flags),
    m_mip_count{mip_count},
    m_sampler_properties{
       FilterType::Linear,
       FilterType::Linear,
       TextureAddressMode::Repeat,
       TextureAddressMode::Repeat,
       TextureAddressMode::Repeat,
       true,
       0.0f,
       0.0f,
       static_cast<float>(mip_count),
    }
{
}

VkImage Texture::vulkan_image() const
{
   if (std::holds_alternative<VkImage>(m_image)) {
      return std::get<VkImage>(m_image);
   }
   return *std::get<vulkan::Image>(m_image);
}

TextureUsageFlags Texture::usage_flags() const
{
   return m_usage_flags;
}

uint32_t Texture::width() const
{
   return m_width;
}

uint32_t Texture::height() const
{
   return m_height;
}

Resolution Texture::resolution() const
{
   return {m_width, m_height};
}

ColorFormat Texture::format() const
{
   return m_color_format;
}

SamplerProperties& Texture::sampler_properties()
{
   return m_sampler_properties;
}

const SamplerProperties& Texture::sampler_properties() const
{
   return m_sampler_properties;
}

Status Texture::write(Device& device, const uint8_t* pixels) const
{
   if (!(this->usage_flags() & TextureUsage::TransferDst)) {
      return Status::InvalidTransferDestination;
   }

   const auto buffer_size = m_color_format.pixel_size() * m_width * m_height;
   auto transfer_buffer = device.create_buffer(BufferUsage::HostVisible | BufferUsage::TransferSrc, buffer_size);
   if (not transfer_buffer.has_value())
      return transfer_buffer.error();

   {
      const auto mapped_memory = transfer_buffer->map_memory();
      if (not mapped_memory.has_value())
         return mapped_memory.error();

      mapped_memory->write(pixels, buffer_size);
   }

   auto one_time_commands = device.create_command_list(WorkType::Graphics);
   if (not one_time_commands.has_value())
      return one_time_commands.error();

   if (const auto res = one_time_commands->begin(SubmitType::OneTime); res != Status::Success)
      return res;

   const TextureBarrierInfo transfer_barrier{
      .texture = this,
      .source_state = TextureState::Undefined,
      .target_state = TextureState::TransferDst,
      .base_mip_level = 0,
      .mip_level_count = m_mip_count,
   };
   one_time_commands->texture_barrier(PipelineStage::Entrypoint, PipelineStage::Transfer, transfer_barrier);

   one_time_commands->copy_buffer_to_texture(*transfer_buffer, *this, 0);

   if (m_mip_count == 1) {
      const TextureBarrierInfo fragment_shader_barrier{
         .texture = this,
         .source_state = TextureState::TransferDst,
         .target_state = TextureState::ShaderRead,
         .base_mip_level = 0,
         .mip_level_count = 1,
      };
      one_time_commands->texture_barrier(PipelineStage::Transfer, PipelineStage::FragmentShader, fragment_shader_barrier);
   } else {
      this->generate_mip_maps_internal(*one_time_commands);
   }

   if (const auto res = one_time_commands->finish(); res != Status::Success)
      return res;

   if (const auto res = device.submit_command_list_one_time(*one_time_commands); res != Status::Success)
      return res;

   return Status::Success;
}

Status Texture::generate_mip_maps(Device& device) const
{
   auto one_time_commands = device.create_command_list();

   if (const auto res = one_time_commands->begin(SubmitType::OneTime); res != Status::Success)
      return res;

   this->generate_mip_maps_internal(*one_time_commands);

   if (const auto res = one_time_commands->finish(); res != Status::Success)
      return res;

   if (const auto res = device.submit_command_list_one_time(*one_time_commands); res != Status::Success)
      return res;

   return Status::Success;
}

Result<TextureView> Texture::create_mip_view(const Device& device, const u32 mip_level) const
{
   assert(mip_level < static_cast<u32>(m_mip_count));

   VkImageViewCreateInfo image_view_info{};
   image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   image_view_info.image = this->vulkan_image();
   image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
   image_view_info.format = *vulkan::to_vulkan_color_format(m_color_format);
   image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
   image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
   image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
   image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
   image_view_info.subresourceRange.aspectMask = vulkan::to_vulkan_image_aspect_flags(m_usage_flags);
   image_view_info.subresourceRange.baseMipLevel = mip_level;
   image_view_info.subresourceRange.levelCount = 1;
   image_view_info.subresourceRange.baseArrayLayer = 0;
   image_view_info.subresourceRange.layerCount = 1;

   vulkan::ImageView image_view(device.vulkan_device());
   if (image_view.construct(&image_view_info) != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   return TextureView(std::move(image_view), m_usage_flags);
}

u32 Texture::mip_count() const
{
   return m_mip_count;
}

TextureView& Texture::view()
{
   return m_texture_view;
}

const TextureView& Texture::view() const
{
   return m_texture_view;
}

void Texture::generate_mip_maps_internal(const CommandList& cmd_list) const
{
   TextureBarrierInfo barrier{
      .texture = this,
      .mip_level_count = 1,
   };

   auto mip_width = static_cast<int>(m_width);
   auto mip_height = static_cast<int>(m_height);

   for (int i = 1; i < m_mip_count; i++) {
      barrier.source_state = TextureState::TransferDst;
      barrier.target_state = TextureState::TransferSrc;
      barrier.base_mip_level = i - 1;
      cmd_list.texture_barrier(PipelineStage::Transfer, PipelineStage::Transfer, barrier);

      TextureRegion src_region{
         .offset_min = {0, 0},
         .offset_max = {mip_width, mip_height},
         .mip_level = i - 1,
      };
      TextureRegion dst_region{
         .offset_min = {0, 0},
         .offset_max = {mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1},
         .mip_level = i,
      };
      cmd_list.blit_texture(*this, src_region, *this, dst_region);

      barrier.source_state = TextureState::TransferSrc;
      barrier.target_state = TextureState::ShaderRead;
      barrier.base_mip_level = i - 1;
      cmd_list.texture_barrier(PipelineStage::Transfer, PipelineStage::FragmentShader, barrier);

      if (mip_width > 1)
         mip_width /= 2;
      if (mip_height > 1)
         mip_height /= 2;
   }

   barrier.source_state = TextureState::TransferDst;
   barrier.target_state = TextureState::ShaderRead;
   barrier.base_mip_level = m_mip_count - 1;
   cmd_list.texture_barrier(PipelineStage::Transfer, PipelineStage::FragmentShader, barrier);
}

VkImageView Texture::vulkan_image_view() const
{
   return m_texture_view.vulkan_image_view();
}

void Texture::set_anisotropy_state(const bool is_enabled)
{
   m_sampler_properties.enable_anisotropy = is_enabled;
}

void Texture::set_lod(const float min, const float max)
{
   m_sampler_properties.min_lod = min;
   m_sampler_properties.max_lod = max;
}

void Texture::set_debug_name(const std::string_view name)
{
   if (name.empty())
      return;
   if (!std::holds_alternative<vulkan::Image>(m_image))
      return;

   auto& image = std::get<vulkan::Image>(m_image);

   VkDebugUtilsObjectNameInfoEXT debug_utils_object_name{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
   debug_utils_object_name.objectHandle = reinterpret_cast<u64>(*image);
   debug_utils_object_name.objectType = VK_OBJECT_TYPE_IMAGE;
   debug_utils_object_name.pObjectName = name.data();
   [[maybe_unused]] const auto result = vulkan::vkSetDebugUtilsObjectNameEXT(image.parent(), &debug_utils_object_name);
   assert(result == VK_SUCCESS);

   std::string view_name{name};
   view_name.append(".view");

   debug_utils_object_name.objectHandle = reinterpret_cast<u64>(m_texture_view.vulkan_image_view());
   debug_utils_object_name.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
   debug_utils_object_name.pObjectName = view_name.data();
   [[maybe_unused]] const auto result2 = vulkan::vkSetDebugUtilsObjectNameEXT(image.parent(), &debug_utils_object_name);
   assert(result2 == VK_SUCCESS);
}

}// namespace triglav::graphics_api
