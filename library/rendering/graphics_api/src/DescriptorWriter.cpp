#include "DescriptorWriter.hpp"

#include "Buffer.hpp"
#include "Sampler.hpp"
#include "Texture.hpp"
#include "vulkan/Util.hpp"

#include "triglav/Int.hpp"
#include "triglav/Ranges.hpp"

#include <Device.hpp>

namespace triglav::graphics_api {

DescriptorWriter::DescriptorWriter(Device& device, const DescriptorView& desc_view) :
    m_device(device),
    m_descriptor_set(desc_view.vulkan_descriptor_set())
{
}

DescriptorWriter::DescriptorWriter(Device& device) :
    m_device(device)
{
}

DescriptorWriter::~DescriptorWriter()
{
   if (m_descriptor_set == nullptr)
      return;

   this->update();

   // for (const auto& write : m_descriptor_writes) {
   //    if (write.descriptor_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
   //       delete[] write.pImageInfo;
   //    }
   // }
}

void DescriptorWriter::set_storage_buffer(uint32_t binding, const Buffer& buffer)
{
   auto& write_descriptor_set = this->write_binding(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

   auto buffer_info = m_descriptor_buffer_info_pool.acquire_object();
   buffer_info->offset = 0;
   buffer_info->range = buffer.size();
   buffer_info->buffer = buffer.vulkan_buffer();
   write_descriptor_set.pBufferInfo = buffer_info;
}

void DescriptorWriter::set_storage_buffer(uint32_t binding, const Buffer& buffer, u32 offset, u32 size)
{
   auto& write_descriptor_set = this->write_binding(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

   auto buffer_info = m_descriptor_buffer_info_pool.acquire_object();
   buffer_info->offset = offset;
   buffer_info->range = size;
   buffer_info->buffer = buffer.vulkan_buffer();
   write_descriptor_set.pBufferInfo = buffer_info;
}

void DescriptorWriter::set_raw_uniform_buffer(const uint32_t binding, const Buffer& buffer)
{
   auto& write_descriptor_set = this->write_binding(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

   auto buffer_info = m_descriptor_buffer_info_pool.acquire_object();
   buffer_info->offset = 0;
   buffer_info->range = buffer.size();
   buffer_info->buffer = buffer.vulkan_buffer();
   write_descriptor_set.pBufferInfo = buffer_info;
}

void DescriptorWriter::set_raw_uniform_buffer(const u32 binding, const Buffer& buffer, const u32 offset, const u32 size)
{
   auto& write_descriptor_set = this->write_binding(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

   auto buffer_info = m_descriptor_buffer_info_pool.acquire_object();
   buffer_info->offset = offset;
   buffer_info->range = size;
   buffer_info->buffer = buffer.vulkan_buffer();
   write_descriptor_set.pBufferInfo = buffer_info;
}

void DescriptorWriter::set_uniform_buffer_array(const uint32_t binding, const std::span<const Buffer*> buffers)
{
   auto& write_descriptor_set = this->write_binding(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
   write_descriptor_set.descriptorCount = static_cast<uint32_t>(buffers.size());

   auto* buffer_infos = new VkDescriptorBufferInfo[buffers.size()];
   for (const auto [i, buffer] : Enumerate(buffers)) {
      buffer_infos[i].offset = 0;
      buffer_infos[i].range = buffer->size();
      buffer_infos[i].buffer = buffer->vulkan_buffer();
   }
   write_descriptor_set.pBufferInfo = buffer_infos;
}

namespace {

VkImageLayout texture_usage_flags_to_vulkan_image_layout(const TextureUsageFlags usage_flags)
{
   if (usage_flags & TextureUsage::DepthStencilAttachment) {
      return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
   }
   return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

}// namespace

void DescriptorWriter::set_sampled_texture(const uint32_t binding, const Texture& texture, const Sampler& sampler)
{
   auto& write_descriptor_set = write_binding(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

   auto image_info = m_descriptor_image_info_pool.acquire_object();
   image_info->imageLayout = texture_usage_flags_to_vulkan_image_layout(texture.usage_flags());
   image_info->imageView = texture.vulkan_image_view();
   image_info->sampler = sampler.vulkan_sampler();

   write_descriptor_set.pImageInfo = image_info;
}

void DescriptorWriter::set_texture_only(uint32_t binding, const Texture& texture)
{
   auto& write_descriptor_set = write_binding(binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

   auto image_info = m_descriptor_image_info_pool.acquire_object();
   image_info->imageLayout = texture_usage_flags_to_vulkan_image_layout(texture.usage_flags());
   image_info->imageView = texture.vulkan_image_view();

   write_descriptor_set.pImageInfo = image_info;
}

void DescriptorWriter::set_texture_view_only(const u32 binding, const TextureView& texture)
{
   auto& write_descriptor_set = write_binding(binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

   auto image_info = m_descriptor_image_info_pool.acquire_object();
   image_info->imageLayout = texture_usage_flags_to_vulkan_image_layout(texture.usage_flags());
   image_info->imageView = texture.vulkan_image_view();

   write_descriptor_set.pImageInfo = image_info;
}

void DescriptorWriter::set_texture_array(const u32 binding, const std::span<const Texture*> textures)
{
   auto& write_descriptor_set = write_binding(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

   write_descriptor_set.descriptorCount = static_cast<u32>(textures.size());

   auto* images = new VkDescriptorImageInfo[textures.size()];
   for (u32 i = 0; i < textures.size(); ++i) {
      auto& image_desc = images[i];
      auto& texture = *textures[i];

      image_desc.imageLayout = texture_usage_flags_to_vulkan_image_layout(texture.usage_flags());
      image_desc.imageView = texture.vulkan_image_view();
      image_desc.sampler = m_device.sampler_cache().find_sampler(texture.sampler_properties()).vulkan_sampler();
   }

   write_descriptor_set.pImageInfo = images;
}

void DescriptorWriter::set_storage_image(uint32_t binding, const Texture& texture)
{
   auto& write_descriptor_set = write_binding(binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

   auto image_info = m_descriptor_image_info_pool.acquire_object();
   image_info->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
   image_info->imageView = texture.vulkan_image_view();

   write_descriptor_set.pImageInfo = image_info;
}

void DescriptorWriter::set_storage_image_view(const uint32_t binding, const TextureView& texture)
{
   auto& write_descriptor_set = write_binding(binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

   auto image_info = m_descriptor_image_info_pool.acquire_object();
   image_info->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
   image_info->imageView = texture.vulkan_image_view();

   write_descriptor_set.pImageInfo = image_info;
}

void DescriptorWriter::set_acceleration_structure(const u32 binding, const ray_tracing::AccelerationStructure& acc_struct)
{
   auto& write_descriptor_set = write_binding(binding, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);

   auto as_info = m_descriptor_acceleration_structure_pool.acquire_object();
   as_info->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
   as_info->accelerationStructureCount = 1;
   as_info->pAccelerationStructures = &acc_struct.vulkan_acceleration_structure();

   write_descriptor_set.pNext = as_info;
}

VkWriteDescriptorSet& DescriptorWriter::write_binding(const u32 binding, VkDescriptorType desc_type)
{
   assert(binding < 16);

   if (m_top_binding < binding) {
      m_top_binding = binding;
   }

   auto& write_descriptor_set = m_descriptor_writes[binding];

   if (write_descriptor_set.pNext != nullptr) {
      m_descriptor_acceleration_structure_pool.release_object(
         static_cast<const VkWriteDescriptorSetAccelerationStructureKHR*>(write_descriptor_set.pNext));
      write_descriptor_set.pNext = nullptr;
   }
   if (write_descriptor_set.pBufferInfo != nullptr) {
      if (write_descriptor_set.descriptorCount > 1) {
         delete[] write_descriptor_set.pBufferInfo;
      } else {
         m_descriptor_buffer_info_pool.release_object(write_descriptor_set.pBufferInfo);
      }
      write_descriptor_set.pBufferInfo = nullptr;
   }
   if (write_descriptor_set.pImageInfo != nullptr) {
      if (write_descriptor_set.descriptorCount > 1) {
         delete[] write_descriptor_set.pImageInfo;
      } else {
         m_descriptor_image_info_pool.release_object(write_descriptor_set.pImageInfo);
      }
      write_descriptor_set.pImageInfo = nullptr;
   }

   write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   write_descriptor_set.dstSet = m_descriptor_set;
   write_descriptor_set.dstBinding = binding;
   write_descriptor_set.dstArrayElement = 0;
   write_descriptor_set.descriptorCount = 1;
   write_descriptor_set.descriptorType = desc_type;

   return write_descriptor_set;
}

VkDescriptorSet DescriptorWriter::vulkan_descriptor_set() const
{
   return m_descriptor_set;
}

std::span<VkWriteDescriptorSet> DescriptorWriter::vulkan_descriptor_writes()
{
   return {m_descriptor_writes.data(), m_top_binding + 1};
}

void DescriptorWriter::update()
{
   assert(m_descriptor_set != nullptr);
   vkUpdateDescriptorSets(m_device.vulkan_device(), m_top_binding + 1, m_descriptor_writes.data(), 0, nullptr);
}

DescriptorWriter::DescriptorWriter(DescriptorWriter&& other) noexcept :
    m_device(other.m_device),
    m_descriptor_set(std::exchange(other.m_descriptor_set, nullptr)),
    m_top_binding(std::exchange(other.m_top_binding, 0)),
    m_descriptor_buffer_info_pool(std::move(other.m_descriptor_buffer_info_pool)),
    m_descriptor_image_info_pool(std::move(other.m_descriptor_image_info_pool)),
    m_descriptor_writes(std::move(other.m_descriptor_writes))
{
}

DescriptorWriter& DescriptorWriter::operator=(DescriptorWriter&& other) noexcept
{
   m_descriptor_set = std::exchange(other.m_descriptor_set, nullptr);
   m_top_binding = std::exchange(other.m_top_binding, 0);
   m_descriptor_writes = other.m_descriptor_writes;
   return *this;
}

void DescriptorWriter::reset_count()
{
   m_top_binding = 0;
}

}// namespace triglav::graphics_api
