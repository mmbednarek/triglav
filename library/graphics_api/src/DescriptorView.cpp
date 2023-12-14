#include "DescriptorView.h"

#include "Buffer.h"
#include "Sampler.h"
#include "Texture.h"
#include "vulkan/Util.h"

namespace graphics_api {

DescriptorView::DescriptorView(const VkDevice device, const VkDescriptorSet descriptorSet,
                               const VkPipelineLayout pipelineLayout) :
    m_device(device),
    m_descriptorSet(descriptorSet),
    m_pipelineLayout(pipelineLayout)
{
}

void DescriptorView::reset()
{
   m_descriptorBufferInfos.clear();
   m_descriptorImageInfos.clear();
   m_descriptorWrites.clear();
}

void DescriptorView::set_raw_uniform_buffer(const uint32_t binding, const Buffer &buffer)
{
   VkWriteDescriptorSet writeDescriptorSet{};
   writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   writeDescriptorSet.dstSet          = m_descriptorSet;
   writeDescriptorSet.dstBinding      = binding;
   writeDescriptorSet.dstArrayElement = 0;
   writeDescriptorSet.descriptorCount = 1;
   writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

   VkDescriptorBufferInfo bufferInfo{};
   bufferInfo.offset              = 0;
   bufferInfo.range               = buffer.size();
   bufferInfo.buffer              = buffer.vulkan_buffer();
   writeDescriptorSet.pBufferInfo = &m_descriptorBufferInfos.emplace_back(bufferInfo);

   m_descriptorWrites.emplace_back(writeDescriptorSet);
}

void DescriptorView::set_sampled_texture(const uint32_t binding, const Texture &texture,
                                         const Sampler &sampler)
{
   VkWriteDescriptorSet writeDescriptorSet{};
   writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   writeDescriptorSet.dstSet          = m_descriptorSet;
   writeDescriptorSet.dstBinding      = binding;
   writeDescriptorSet.dstArrayElement = 0;
   writeDescriptorSet.descriptorCount = 1;
   writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

   VkDescriptorImageInfo imageInfo{};
   imageInfo.imageLayout         = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   imageInfo.imageView           = texture.vulkan_image_view();
   imageInfo.sampler             = sampler.vulkan_sampler();
   writeDescriptorSet.pImageInfo = &m_descriptorImageInfos.emplace_back(imageInfo);

   m_descriptorWrites.emplace_back(writeDescriptorSet);
}

void DescriptorView::update()
{
   vkUpdateDescriptorSets(m_device, m_descriptorWrites.size(), m_descriptorWrites.data(), 0, nullptr);
   this->reset();
}

VkDescriptorSet DescriptorView::vulkan_descriptor_set() const
{
   return m_descriptorSet;
}

VkPipelineLayout DescriptorView::vulkan_pipeline_layout() const
{
   return m_pipelineLayout;
}


}// namespace graphics_api
