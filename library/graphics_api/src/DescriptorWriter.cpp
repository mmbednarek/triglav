#include "DescriptorWriter.hpp"

#include "Buffer.hpp"
#include "Sampler.hpp"
#include "Texture.hpp"
#include "vulkan/Util.hpp"

#include "triglav/Int.hpp"
#include "triglav/Ranges.hpp"

#include <Device.hpp>

namespace triglav::graphics_api {

DescriptorWriter::DescriptorWriter(Device& device, const DescriptorView& descView) :
    m_device(device),
    m_descriptorSet(descView.vulkan_descriptor_set())
{
}

DescriptorWriter::DescriptorWriter(Device& device) :
    m_device(device)
{
}

DescriptorWriter::~DescriptorWriter()
{
   if (m_descriptorSet == nullptr)
      return;

   this->update();
}

void DescriptorWriter::set_storage_buffer(uint32_t binding, const Buffer& buffer)
{
   auto& writeDescriptorSet = this->write_binding(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

   auto bufferInfo = m_descriptorBufferInfoPool.acquire_object();
   bufferInfo->offset = 0;
   bufferInfo->range = buffer.size();
   bufferInfo->buffer = buffer.vulkan_buffer();
   writeDescriptorSet.pBufferInfo = bufferInfo;
}

void DescriptorWriter::set_storage_buffer(uint32_t binding, const Buffer& buffer, u32 offset, u32 size)
{
   auto& writeDescriptorSet = this->write_binding(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

   auto bufferInfo = m_descriptorBufferInfoPool.acquire_object();
   bufferInfo->offset = offset;
   bufferInfo->range = size;
   bufferInfo->buffer = buffer.vulkan_buffer();
   writeDescriptorSet.pBufferInfo = bufferInfo;
}

void DescriptorWriter::set_raw_uniform_buffer(const uint32_t binding, const Buffer& buffer)
{
   auto& writeDescriptorSet = this->write_binding(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

   auto bufferInfo = m_descriptorBufferInfoPool.acquire_object();
   bufferInfo->offset = 0;
   bufferInfo->range = buffer.size();
   bufferInfo->buffer = buffer.vulkan_buffer();
   writeDescriptorSet.pBufferInfo = bufferInfo;
}

void DescriptorWriter::set_uniform_buffer_array(const uint32_t binding, const std::span<const Buffer*> buffers)
{
   auto& writeDescriptorSet = this->write_binding(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
   writeDescriptorSet.descriptorCount = static_cast<uint32_t>(buffers.size());

   auto* bufferInfos = new VkDescriptorBufferInfo[buffers.size()];
   for (const auto [i, buffer] : Enumerate(buffers)) {
      bufferInfos[i].offset = 0;
      bufferInfos[i].range = buffer->size();
      bufferInfos[i].buffer = buffer->vulkan_buffer();
   }
   writeDescriptorSet.pBufferInfo = bufferInfos;
}

namespace {

VkImageLayout texture_usage_flags_to_vulkan_image_layout(const TextureUsageFlags usageFlags)
{
   if (usageFlags & TextureUsage::DepthStencilAttachment) {
      return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
   }
   return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

}// namespace

void DescriptorWriter::set_sampled_texture(const uint32_t binding, const Texture& texture, const Sampler& sampler)
{
   auto& writeDescriptorSet = write_binding(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

   auto imageInfo = m_descriptorImageInfoPool.acquire_object();
   imageInfo->imageLayout = texture_usage_flags_to_vulkan_image_layout(texture.usage_flags());
   imageInfo->imageView = texture.vulkan_image_view();
   imageInfo->sampler = sampler.vulkan_sampler();

   writeDescriptorSet.pImageInfo = imageInfo;
}

void DescriptorWriter::set_texture_only(uint32_t binding, const Texture& texture)
{
   auto& writeDescriptorSet = write_binding(binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

   auto imageInfo = m_descriptorImageInfoPool.acquire_object();
   imageInfo->imageLayout = texture_usage_flags_to_vulkan_image_layout(texture.usage_flags());
   imageInfo->imageView = texture.vulkan_image_view();

   writeDescriptorSet.pImageInfo = imageInfo;
}

void DescriptorWriter::set_texture_view_only(const uint32_t binding, const TextureView& texture)
{
   auto& writeDescriptorSet = write_binding(binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

   auto imageInfo = m_descriptorImageInfoPool.acquire_object();
   imageInfo->imageLayout = texture_usage_flags_to_vulkan_image_layout(texture.usage_flags());
   imageInfo->imageView = texture.vulkan_image_view();

   writeDescriptorSet.pImageInfo = imageInfo;
}

void DescriptorWriter::set_texture_array(uint32_t binding, const std::span<const Texture*> textures)
{
   auto& writeDescriptorSet = write_binding(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

   writeDescriptorSet.descriptorCount = textures.size();

   auto* images = new VkDescriptorImageInfo[textures.size()];
   for (u32 i = 0; i < textures.size(); ++i) {
      auto& imageDesc = images[i];
      auto& texture = *textures[i];

      imageDesc.imageLayout = texture_usage_flags_to_vulkan_image_layout(texture.usage_flags());
      imageDesc.imageView = texture.vulkan_image_view();
      imageDesc.sampler = m_device.sampler_cache().find_sampler(texture.sampler_properties()).vulkan_sampler();
   }

   writeDescriptorSet.pImageInfo = images;
}

void DescriptorWriter::set_storage_image(uint32_t binding, const Texture& texture)
{
   auto& writeDescriptorSet = write_binding(binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

   auto imageInfo = m_descriptorImageInfoPool.acquire_object();
   imageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
   imageInfo->imageView = texture.vulkan_image_view();

   writeDescriptorSet.pImageInfo = imageInfo;
}

void DescriptorWriter::set_storage_image_view(const uint32_t binding, const TextureView& texture)
{
   auto& writeDescriptorSet = write_binding(binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

   auto imageInfo = m_descriptorImageInfoPool.acquire_object();
   imageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
   imageInfo->imageView = texture.vulkan_image_view();

   writeDescriptorSet.pImageInfo = imageInfo;
}

void DescriptorWriter::set_acceleration_structure(uint32_t binding, const ray_tracing::AccelerationStructure& accStruct)
{
   auto& writeDescriptorSet = write_binding(binding, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);

   auto asInfo = m_descriptorAccelerationStructurePool.acquire_object();
   asInfo->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
   asInfo->accelerationStructureCount = 1;
   asInfo->pAccelerationStructures = &accStruct.vulkan_acceleration_structure();

   writeDescriptorSet.pNext = asInfo;
}

VkWriteDescriptorSet& DescriptorWriter::write_binding(const u32 binding, VkDescriptorType descType)
{
   assert(binding < 16);

   if (m_topBinding < binding) {
      m_topBinding = binding;
   }

   auto& writeDescriptorSet = m_descriptorWrites[binding];

   if (writeDescriptorSet.pNext != nullptr) {
      m_descriptorAccelerationStructurePool.release_object(
         static_cast<const VkWriteDescriptorSetAccelerationStructureKHR*>(writeDescriptorSet.pNext));
      writeDescriptorSet.pNext = nullptr;
   }
   if (writeDescriptorSet.pBufferInfo != nullptr) {
      if (writeDescriptorSet.descriptorCount > 1) {
         delete[] writeDescriptorSet.pBufferInfo;
      } else {
         m_descriptorBufferInfoPool.release_object(writeDescriptorSet.pBufferInfo);
      }
      writeDescriptorSet.pBufferInfo = nullptr;
   }
   if (writeDescriptorSet.pImageInfo != nullptr) {
      if (writeDescriptorSet.descriptorCount > 1) {
         delete[] writeDescriptorSet.pImageInfo;
      } else {
         m_descriptorImageInfoPool.release_object(writeDescriptorSet.pImageInfo);
      }
      writeDescriptorSet.pImageInfo = nullptr;
   }

   writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   writeDescriptorSet.dstSet = m_descriptorSet;
   writeDescriptorSet.dstBinding = binding;
   writeDescriptorSet.dstArrayElement = 0;
   writeDescriptorSet.descriptorCount = 1;
   writeDescriptorSet.descriptorType = descType;

   return writeDescriptorSet;
}

VkDescriptorSet DescriptorWriter::vulkan_descriptor_set() const
{
   return m_descriptorSet;
}

std::span<VkWriteDescriptorSet> DescriptorWriter::vulkan_descriptor_writes()
{
   return {m_descriptorWrites.data(), m_topBinding + 1};
}

void DescriptorWriter::update()
{
   assert(m_descriptorSet != nullptr);
   vkUpdateDescriptorSets(m_device.vulkan_device(), m_topBinding + 1, m_descriptorWrites.data(), 0, nullptr);
}

DescriptorWriter::DescriptorWriter(DescriptorWriter&& other) noexcept :
    m_device(other.m_device),
    m_descriptorSet(std::exchange(other.m_descriptorSet, nullptr)),
    m_topBinding(std::exchange(other.m_topBinding, 0)),
    m_descriptorBufferInfoPool(std::move(other.m_descriptorBufferInfoPool)),
    m_descriptorImageInfoPool(std::move(other.m_descriptorImageInfoPool)),
    m_descriptorWrites(std::move(other.m_descriptorWrites))
{
}

DescriptorWriter& DescriptorWriter::operator=(DescriptorWriter&& other) noexcept
{
   m_descriptorSet = std::exchange(other.m_descriptorSet, nullptr);
   m_topBinding = std::exchange(other.m_topBinding, 0);
   m_descriptorWrites = other.m_descriptorWrites;
   return *this;
}

void DescriptorWriter::reset_count()
{
   m_topBinding = 0;
}

}// namespace triglav::graphics_api
