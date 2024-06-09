#include "Texture.h"

#include "CommandList.h"
#include "Device.h"

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

}// namespace triglav::graphics_api
