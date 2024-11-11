#include "DescriptorPool.hpp"

namespace triglav::graphics_api {

DescriptorPool::DescriptorPool(vulkan::DescriptorPool descriptorPool, const VkDescriptorSetLayout descriptorSetLayout) :
    m_descriptorPool(std::move(descriptorPool)),
    m_descriptorSetLayout(descriptorSetLayout)
{
}

Status DescriptorPool::allocate_descriptors(const std::span<VkDescriptorSetLayout> inLayouts, std::span<VkDescriptorSet> outSets)
{
   VkDescriptorSetAllocateInfo descriptorSetsInfo{};
   descriptorSetsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   descriptorSetsInfo.descriptorPool = *m_descriptorPool;
   descriptorSetsInfo.descriptorSetCount = static_cast<u32>(inLayouts.size());
   descriptorSetsInfo.pSetLayouts = inLayouts.data();

   if (const auto res = vkAllocateDescriptorSets(m_descriptorPool.parent(), &descriptorSetsInfo, outSets.data()); res != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }

   return Status::Success;
}

Status DescriptorPool::free_descriptors(const std::span<VkDescriptorSet> sets)
{
   const auto res = vkFreeDescriptorSets(m_descriptorPool.parent(), *m_descriptorPool, sets.size(), sets.data());
   if (res != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }
   return Status::Success;
}

Result<DescriptorArray> DescriptorPool::allocate_array(const size_t descriptorCount)
{
   std::vector<VkDescriptorSetLayout> descriptorLayouts{};
   descriptorLayouts.resize(descriptorCount, m_descriptorSetLayout);

   VkDescriptorSetAllocateInfo descriptorSetsInfo{};
   descriptorSetsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   descriptorSetsInfo.descriptorPool = *m_descriptorPool;
   descriptorSetsInfo.descriptorSetCount = descriptorLayouts.size();
   descriptorSetsInfo.pSetLayouts = descriptorLayouts.data();

   std::vector<VkDescriptorSet> descriptorSets{};
   descriptorSets.resize(descriptorCount);
   if (const auto res = vkAllocateDescriptorSets(m_descriptorPool.parent(), &descriptorSetsInfo, descriptorSets.data());
       res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return DescriptorArray(m_descriptorPool.parent(), *m_descriptorPool, std::move(descriptorSets));
}

}// namespace triglav::graphics_api