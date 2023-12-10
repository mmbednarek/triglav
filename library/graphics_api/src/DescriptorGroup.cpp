#include "DescriptorGroup.h"

#include "Buffer.h"
#include "Texture.h"
#include "vulkan/Util.h"

namespace graphics_api {

DescriptorGroup::DescriptorGroup(VkDevice device, VkDescriptorPool descriptorPool, VkSampler sampler,
                                 VkPipelineLayout pipelineLayout,
                                 std::vector<VkDescriptorSet> descriptorSets) :
    m_device(device),
    m_descriptorPool(descriptorPool),
    m_sampler(sampler),
    m_pipelineLayout(pipelineLayout),
    m_descriptorSets(std::move(descriptorSets))
{
}

DescriptorGroup::DescriptorGroup(DescriptorGroup &&other) noexcept :
    m_device(std::exchange(other.m_device, nullptr)),
    m_descriptorPool(std::exchange(other.m_descriptorPool, nullptr)),
    m_sampler(std::exchange(other.m_sampler, nullptr)),
    m_pipelineLayout(std::exchange(other.m_pipelineLayout, nullptr)),
    m_descriptorSets(std::move(other.m_descriptorSets))
{
}

DescriptorGroup &DescriptorGroup::operator=(DescriptorGroup &&other) noexcept
{
   m_device         = std::exchange(other.m_device, nullptr);
   m_descriptorPool = std::exchange(other.m_descriptorPool, nullptr);
   m_sampler        = std::exchange(other.m_sampler, nullptr);
   m_pipelineLayout = std::exchange(other.m_pipelineLayout, nullptr);
   m_descriptorSets = std::move(other.m_descriptorSets);
   return *this;
}

DescriptorGroup::~DescriptorGroup()
{
   if (not m_descriptorSets.empty()) {
      vkFreeDescriptorSets(m_device, m_descriptorPool, m_descriptorSets.size(), m_descriptorSets.data());
   }
}

size_t DescriptorGroup::count() const
{
   return m_descriptorSets.size();
}

VkPipelineLayout DescriptorGroup::pipeline_layout() const
{
   return m_pipelineLayout;
}

const VkDescriptorSet &DescriptorGroup::at(const size_t index) const
{
   return m_descriptorSets.at(index);
}

void DescriptorGroup::update(const size_t descriptorId, const std::span<DescriptorWrite> descriptors) const
{
   std::vector<VkDescriptorBufferInfo> descriptorBufferInfos{};
   std::vector<VkDescriptorImageInfo> descriptorImageInfos{};

   std::vector<VkWriteDescriptorSet> descriptorWrites{};

   for (const auto &descriptor : descriptors) {
      VkWriteDescriptorSet writeDescriptorSet{};
      writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSet.dstSet          = m_descriptorSets[descriptorId];
      writeDescriptorSet.dstBinding      = descriptor.binding;
      writeDescriptorSet.dstArrayElement = 0;
      writeDescriptorSet.descriptorCount = 1;
      writeDescriptorSet.descriptorType  = vulkan::to_vulkan_descriptor_type(descriptor.type);

      if (descriptor.type == DescriptorType::UniformBuffer) {
         const auto *buffer = std::get<const Buffer *>(descriptor.data);
         VkDescriptorBufferInfo bufferInfo{};
         bufferInfo.offset              = 0;
         bufferInfo.range               = buffer->size();
         bufferInfo.buffer              = buffer->vulkan_buffer();
         writeDescriptorSet.pBufferInfo = &descriptorBufferInfos.emplace_back(bufferInfo);
      } else if (descriptor.type == DescriptorType::ImageSampler) {
         const auto *texture = std::get<const Texture *>(descriptor.data);
         VkDescriptorImageInfo imageInfo{};
         imageInfo.imageLayout         = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
         imageInfo.imageView           = texture->vulkan_image_view();
         imageInfo.sampler             = m_sampler;
         writeDescriptorSet.pImageInfo = &descriptorImageInfos.emplace_back(imageInfo);
      }

      descriptorWrites.emplace_back(writeDescriptorSet);
   }

   vkUpdateDescriptorSets(m_device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

}// namespace graphics_api