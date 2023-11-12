#include "Pipeline.h"
#include "Buffer.h"
#include "Texture.h"
#include "vulkan/Util.h"

namespace graphics_api {

Pipeline::Pipeline(vulkan::PipelineLayout layout, vulkan::Pipeline pipeline,
                   vulkan::DescriptorPool descriptorPool, vulkan::DescriptorSetLayout descriptorSetLayout,
                   std::vector<VkDescriptorSet> descriptorSets, VkSampler sampler) :
    m_layout(std::move(layout)),
    m_pipeline(std::move(pipeline)),
    m_descriptorPool(std::move(descriptorPool)),
    m_descriptorSetLayout(std::move(descriptorSetLayout)),
    m_descriptorSets(std::move(descriptorSets)),
    m_sampler(sampler)
{
}

Pipeline::Pipeline(Pipeline &&other) noexcept :
    m_layout(std::move(other.m_layout)),
    m_pipeline(std::move(other.m_pipeline)),
    m_descriptorPool(std::move(other.m_descriptorPool)),
    m_descriptorSetLayout(std::move(other.m_descriptorSetLayout)),
    m_descriptorSets(std::move(other.m_descriptorSets)),
    m_sampler(other.m_sampler)
{
}

Pipeline &Pipeline::operator=(Pipeline &&other) noexcept
{
   m_layout              = std::move(other.m_layout);
   m_pipeline            = std::move(other.m_pipeline);
   m_descriptorPool      = std::move(other.m_descriptorPool);
   m_descriptorSetLayout = std::move(other.m_descriptorSetLayout);
   m_descriptorSets      = std::move(other.m_descriptorSets);
   m_sampler             = other.m_sampler;
   return *this;
}

VkPipeline Pipeline::vulkan_pipeline() const
{
   return *m_pipeline;
}

void Pipeline::update_descriptors(size_t descriptorId, std::span<DescriptorWrite> descriptors)
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
         const auto *buffer = std::get<Buffer *>(descriptor.data);
         VkDescriptorBufferInfo bufferInfo{};
         bufferInfo.offset              = 0;
         bufferInfo.range               = buffer->size();
         bufferInfo.buffer              = buffer->vulkan_buffer();
         writeDescriptorSet.pBufferInfo = &descriptorBufferInfos.emplace_back(bufferInfo);
      } else if (descriptor.type == DescriptorType::ImageSampler) {
         const auto *texture = std::get<Texture *>(descriptor.data);
         VkDescriptorImageInfo imageInfo{};
         imageInfo.imageLayout         = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
         imageInfo.imageView           = texture->vulkan_image_view();
         imageInfo.sampler             = m_sampler;
         writeDescriptorSet.pImageInfo = &descriptorImageInfos.emplace_back(imageInfo);
      }

      descriptorWrites.emplace_back(writeDescriptorSet);
   }

   vkUpdateDescriptorSets(m_pipeline.parent(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

Pipeline::~Pipeline()
{
   if (not m_descriptorSets.empty()) {
      vkFreeDescriptorSets(m_pipeline.parent(), *m_descriptorPool, m_descriptorSets.size(),
                           m_descriptorSets.data());
   }
}

const vulkan::PipelineLayout &Pipeline::layout() const
{
   return m_layout;
}

const std::vector<VkDescriptorSet> &Pipeline::descriptor_sets() const
{
   return m_descriptorSets;
}

size_t Pipeline::descriptor_set_count() const
{
   return m_descriptorSets.size();
}


}// namespace graphics_api
