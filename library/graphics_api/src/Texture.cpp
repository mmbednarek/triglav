#include "Texture.hpp"

#include "CommandList.hpp"
#include "Device.hpp"
#include "vulkan/Util.hpp"

namespace triglav::graphics_api {

Texture::Texture(vulkan::Image image, vulkan::DeviceMemory memory, vulkan::ImageView imageView, const ColorFormat& colorFormat,
                 const TextureUsageFlags usageFlags, const uint32_t width, const uint32_t height, const int mipCount) :
    m_width{width},
    m_height{height},
    m_colorFormat(colorFormat),
    m_usageFlags(usageFlags),
    m_image(std::move(image)),
    m_memory(std::move(memory)),
    m_imageView(std::move(imageView)),
    m_mipCount{mipCount},
    m_samplerProperties{
       FilterType::Linear,
       FilterType::Linear,
       TextureAddressMode::Repeat,
       TextureAddressMode::Repeat,
       TextureAddressMode::Repeat,
       true,
       0.0f,
       0.0f,
       static_cast<float>(mipCount),
    }
{
}

VkImage Texture::vulkan_image() const
{
   return *m_image;
}

TextureUsageFlags Texture::usage_flags() const
{
   return m_usageFlags;
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

const SamplerProperties& Texture::sampler_properties() const
{
   return m_samplerProperties;
}

Status Texture::write(Device& device, const uint8_t* pixels) const
{
   if (!(this->usage_flags() & TextureUsage::TransferDst)) {
      return Status::InvalidTransferDestination;
   }

   const auto bufferSize = m_colorFormat.pixel_size() * m_width * m_height;
   auto transferBuffer = device.create_buffer(BufferUsage::HostVisible | BufferUsage::TransferSrc, bufferSize);
   if (not transferBuffer.has_value())
      return transferBuffer.error();

   {
      const auto mappedMemory = transferBuffer->map_memory();
      if (not mappedMemory.has_value())
         return mappedMemory.error();

      mappedMemory->write(pixels, bufferSize);
   }

   auto oneTimeCommands = device.create_command_list(WorkType::Graphics);
   if (not oneTimeCommands.has_value())
      return oneTimeCommands.error();

   if (const auto res = oneTimeCommands->begin(SubmitType::OneTime); res != Status::Success)
      return res;

   const TextureBarrierInfo transferBarrier{
      .texture = this,
      .sourceState = TextureState::Undefined,
      .targetState = TextureState::TransferDst,
      .baseMipLevel = 0,
      .mipLevelCount = m_mipCount,
   };
   oneTimeCommands->texture_barrier(PipelineStage::Entrypoint, PipelineStage::Transfer, transferBarrier);

   oneTimeCommands->copy_buffer_to_texture(*transferBuffer, *this, 0);

   if (m_mipCount == 1) {
      const TextureBarrierInfo fragmentShaderBarrier{
         .texture = this,
         .sourceState = TextureState::TransferDst,
         .targetState = TextureState::ShaderRead,
         .baseMipLevel = 0,
         .mipLevelCount = 1,
      };
      oneTimeCommands->texture_barrier(PipelineStage::Transfer, PipelineStage::FragmentShader, fragmentShaderBarrier);
   } else {
      this->generate_mip_maps_internal(*oneTimeCommands);
   }

   if (const auto res = oneTimeCommands->finish(); res != Status::Success)
      return res;

   if (const auto res = device.submit_command_list_one_time(*oneTimeCommands); res != Status::Success)
      return res;

   return Status::Success;
}

Status Texture::generate_mip_maps(Device& device) const
{
   auto oneTimeCommands = device.create_command_list();

   if (const auto res = oneTimeCommands->begin(SubmitType::OneTime); res != Status::Success)
      return res;

   this->generate_mip_maps_internal(*oneTimeCommands);

   if (const auto res = oneTimeCommands->finish(); res != Status::Success)
      return res;

   if (const auto res = device.submit_command_list_one_time(*oneTimeCommands); res != Status::Success)
      return res;

   return Status::Success;
}

Result<TextureView> Texture::create_mip_view(const Device& device, const u32 mipLevel) const
{
   VkImageViewCreateInfo imageViewInfo{};
   imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   imageViewInfo.image = *m_image;
   imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
   imageViewInfo.format = *vulkan::to_vulkan_color_format(m_colorFormat);
   imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
   imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
   imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
   imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
   imageViewInfo.subresourceRange.aspectMask = vulkan::to_vulkan_image_aspect_flags(m_usageFlags);
   imageViewInfo.subresourceRange.baseMipLevel = mipLevel;
   imageViewInfo.subresourceRange.levelCount = 1;
   imageViewInfo.subresourceRange.baseArrayLayer = 0;
   imageViewInfo.subresourceRange.layerCount = 1;

   vulkan::ImageView imageView(device.vulkan_device());
   if (imageView.construct(&imageViewInfo) != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   return TextureView(std::move(imageView), m_usageFlags);
}

void Texture::generate_mip_maps_internal(const CommandList& cmdList) const
{
   TextureBarrierInfo barrier{
      .texture = this,
      .mipLevelCount = 1,
   };

   auto mipWidth = static_cast<int>(m_width);
   auto mipHeight = static_cast<int>(m_height);

   for (int i = 1; i < m_mipCount; i++) {
      barrier.sourceState = TextureState::TransferDst;
      barrier.targetState = TextureState::TransferSrc;
      barrier.baseMipLevel = i - 1;
      cmdList.texture_barrier(PipelineStage::Transfer, PipelineStage::Transfer, barrier);

      TextureRegion srcRegion{
         .offsetMin = {0, 0},
         .offsetMax = {mipWidth, mipHeight},
         .mipLevel = i - 1,
      };
      TextureRegion dstRegion{
         .offsetMin = {0, 0},
         .offsetMax = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1},
         .mipLevel = i,
      };
      cmdList.blit_texture(*this, srcRegion, *this, dstRegion);

      barrier.sourceState = TextureState::TransferSrc;
      barrier.targetState = TextureState::ShaderRead;
      barrier.baseMipLevel = i - 1;
      cmdList.texture_barrier(PipelineStage::Transfer, PipelineStage::FragmentShader, barrier);

      if (mipWidth > 1)
         mipWidth /= 2;
      if (mipHeight > 1)
         mipHeight /= 2;
   }

   barrier.sourceState = TextureState::TransferDst;
   barrier.targetState = TextureState::ShaderRead;
   barrier.baseMipLevel = m_mipCount - 1;
   cmdList.texture_barrier(PipelineStage::Transfer, PipelineStage::FragmentShader, barrier);
}

VkImageView Texture::vulkan_image_view() const
{
   return *m_imageView;
}

void Texture::set_anisotropy_state(const bool isEnabled)
{
   m_samplerProperties.enableAnisotropy = isEnabled;
}

void Texture::set_lod(const float min, const float max)
{
   m_samplerProperties.minLod = min;
   m_samplerProperties.maxLod = max;
}

void Texture::set_debug_name(const std::string_view name)
{
   if (name.empty())
      return;

   VkDebugUtilsObjectNameInfoEXT debugUtilsObjectName{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
   debugUtilsObjectName.objectHandle = reinterpret_cast<u64>(*m_image);
   debugUtilsObjectName.objectType = VK_OBJECT_TYPE_IMAGE;
   debugUtilsObjectName.pObjectName = name.data();
   const auto result = vulkan::vkSetDebugUtilsObjectNameEXT(m_image.parent(), &debugUtilsObjectName);
   assert(result == VK_SUCCESS);

   std::string viewName{"VIEW_"};
   viewName.append(name);

   debugUtilsObjectName.objectHandle = reinterpret_cast<u64>(*m_imageView);
   debugUtilsObjectName.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
   debugUtilsObjectName.pObjectName = viewName.data();
   const auto result2 = vulkan::vkSetDebugUtilsObjectNameEXT(m_imageView.parent(), &debugUtilsObjectName);
   assert(result2 == VK_SUCCESS);
}

}// namespace triglav::graphics_api
