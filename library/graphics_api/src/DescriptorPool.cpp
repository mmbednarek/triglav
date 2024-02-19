#include "DescriptorPool.h"

namespace triglav::graphics_api {

DescriptorPool::DescriptorPool(vulkan::DescriptorPool descriptorPool,
                               const VkDescriptorSetLayout descriptorSetLayout,
                               const VkPipelineLayout pipelineLayout) :
    m_descriptorPool(std::move(descriptorPool)),
    m_descriptorSetLayout(descriptorSetLayout),
    m_pipelineLayout(pipelineLayout)
{
}

Result<DescriptorArray> DescriptorPool::allocate_array(const size_t descriptorCount)
{
   std::vector<VkDescriptorSetLayout> descriptorLayouts{};
   descriptorLayouts.resize(descriptorCount, m_descriptorSetLayout);

   VkDescriptorSetAllocateInfo descriptorSetsInfo{};
   descriptorSetsInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   descriptorSetsInfo.descriptorPool     = *m_descriptorPool;
   descriptorSetsInfo.descriptorSetCount = descriptorLayouts.size();
   descriptorSetsInfo.pSetLayouts        = descriptorLayouts.data();

   std::vector<VkDescriptorSet> descriptorSets{};
   descriptorSets.resize(descriptorCount);
   if (const auto res = vkAllocateDescriptorSets(m_descriptorPool.parent(), &descriptorSetsInfo,
                                                 descriptorSets.data());
       res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return DescriptorArray(m_descriptorPool.parent(), *m_descriptorPool, m_pipelineLayout, std::move(descriptorSets));
}

}// namespace graphics_api