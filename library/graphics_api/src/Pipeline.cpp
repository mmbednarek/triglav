#include "Pipeline.h"
#include "Buffer.h"
#include "vulkan/Util.h"

namespace graphics_api {

Pipeline::Pipeline(vulkan::PipelineLayout layout, vulkan::Pipeline pipeline,
                   vulkan::DescriptorPool descriptorPool, vulkan::DescriptorSetLayout descriptorSetLayout,
                   const VkSampler sampler) :
    m_layout(std::move(layout)),
    m_pipeline(std::move(pipeline)),
    m_descriptorPool(std::move(descriptorPool)),
    m_descriptorSetLayout(std::move(descriptorSetLayout)),
    m_sampler(sampler)
{
}

VkPipeline Pipeline::vulkan_pipeline() const
{
   return *m_pipeline;
}

const vulkan::PipelineLayout &Pipeline::layout() const
{
   return m_layout;
}

Result<DescriptorGroup> Pipeline::allocate_descriptors(const size_t descriptorCount)
{
   std::vector<VkDescriptorSetLayout> descriptorLayouts{};
   descriptorLayouts.resize(descriptorCount, *m_descriptorSetLayout);

   VkDescriptorSetAllocateInfo descriptorSetsInfo{};
   descriptorSetsInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   descriptorSetsInfo.descriptorPool     = *m_descriptorPool;
   descriptorSetsInfo.descriptorSetCount = descriptorLayouts.size();
   descriptorSetsInfo.pSetLayouts        = descriptorLayouts.data();

   std::vector<VkDescriptorSet> descriptorSets{};
   descriptorSets.resize(descriptorCount);
   if (const auto res = vkAllocateDescriptorSets(m_pipeline.parent(), &descriptorSetsInfo, descriptorSets.data());
       res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return DescriptorGroup(m_pipeline.parent(), *m_descriptorPool, m_sampler, *m_layout, std::move(descriptorSets));
}


}// namespace graphics_api
