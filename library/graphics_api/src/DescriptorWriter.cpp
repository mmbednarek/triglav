#include "DescriptorWriter.h"

#include "Buffer.h"
#include "Sampler.h"
#include "Texture.h"
#include "vulkan/Util.h"

#include <Device.h>

namespace triglav::graphics_api {

DescriptorWriter::DescriptorWriter(const Device &device, const DescriptorView &descView) :
    m_device(device.vulkan_device()),
    m_descriptorSet(descView.vulkan_descriptor_set())
{
}

DescriptorWriter::DescriptorWriter(const Device &device) :
    m_device(device.vulkan_device())
{
}

DescriptorWriter::~DescriptorWriter()
{
   if (m_descriptorSet == nullptr)
      return;

   this->update();
}

void DescriptorWriter::set_storage_buffer(uint32_t binding, const Buffer &buffer)
{
   auto &writeDescriptorSet = this->write_binding(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

   auto bufferInfo                = m_descriptorBufferInfoPool.aquire_object();
   bufferInfo->offset             = 0;
   bufferInfo->range              = buffer.size();
   bufferInfo->buffer             = buffer.vulkan_buffer();
   writeDescriptorSet.pBufferInfo = bufferInfo;
}

void DescriptorWriter::set_raw_uniform_buffer(const uint32_t binding, const Buffer &buffer)
{
   auto &writeDescriptorSet = this->write_binding(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

   auto bufferInfo                = m_descriptorBufferInfoPool.aquire_object();
   bufferInfo->offset             = 0;
   bufferInfo->range              = buffer.size();
   bufferInfo->buffer             = buffer.vulkan_buffer();
   writeDescriptorSet.pBufferInfo = bufferInfo;
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

void DescriptorWriter::set_sampled_texture(const uint32_t binding, const Texture &texture,
                                           const Sampler &sampler)
{
   auto &writeDescriptorSet = write_binding(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

   auto imageInfo         = m_descriptorImageInfoPool.aquire_object();
   imageInfo->imageLayout = texture_usage_flags_to_vulkan_image_layout(texture.usage_flags());
   imageInfo->imageView   = texture.vulkan_image_view();
   imageInfo->sampler     = sampler.vulkan_sampler();

   writeDescriptorSet.pImageInfo = imageInfo;
}

VkWriteDescriptorSet &DescriptorWriter::write_binding(const u32 binding, VkDescriptorType descType)
{
   assert(binding < 16);

   if (m_topBinding < binding) {
      m_topBinding = binding;
   }

   auto &writeDescriptorSet = m_descriptorWrites[binding];

   if (writeDescriptorSet.pBufferInfo != nullptr) {
      m_descriptorBufferInfoPool.release_object(writeDescriptorSet.pBufferInfo);
   }
   if (writeDescriptorSet.pImageInfo != nullptr) {
      m_descriptorImageInfoPool.release_object(writeDescriptorSet.pImageInfo);
   }

   writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   writeDescriptorSet.dstSet          = m_descriptorSet;
   writeDescriptorSet.dstBinding      = binding;
   writeDescriptorSet.dstArrayElement = 0;
   writeDescriptorSet.descriptorCount = 1;
   writeDescriptorSet.descriptorType  = descType;

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
   vkUpdateDescriptorSets(m_device, m_topBinding + 1, m_descriptorWrites.data(), 0, nullptr);
}

DescriptorWriter::DescriptorWriter(DescriptorWriter &&other) noexcept :
    m_device(std::exchange(other.m_device, nullptr)),
    m_descriptorSet(std::exchange(other.m_descriptorSet, nullptr)),
    m_topBinding(std::exchange(other.m_topBinding, 0)),
    m_descriptorBufferInfoPool(std::move(other.m_descriptorBufferInfoPool)),
    m_descriptorImageInfoPool(std::move(other.m_descriptorImageInfoPool)),
    m_descriptorWrites(std::move(other.m_descriptorWrites))
{
}

DescriptorWriter &DescriptorWriter::operator=(DescriptorWriter &&other) noexcept
{
   m_device                   = std::exchange(other.m_device, nullptr);
   m_descriptorSet            = std::exchange(other.m_descriptorSet, nullptr);
   m_topBinding               = std::exchange(other.m_topBinding, 0);
   m_descriptorWrites         = other.m_descriptorWrites;
   return *this;
}

void DescriptorWriter::reset_count() {
   m_topBinding = 0;
}

}// namespace triglav::graphics_api
