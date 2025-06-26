#include "DescriptorPool.hpp"
#include "Pipeline.hpp"

#include <DescriptorLayoutCache.hpp>

namespace triglav::graphics_api {

const std::vector<VkDescriptorSetLayout>& DescriptorLayoutArray::layouts() const
{
   return m_layouts;
}

void DescriptorLayoutArray::add_from_cache(DescriptorLayoutCache& cache, const std::span<DescriptorBinding> bindings)
{
   m_layouts.push_back(cache.find_layout(bindings));
}

void DescriptorLayoutArray::add_from_pipeline(const Pipeline& pipeline)
{
   m_layouts.push_back(pipeline.vulkan_descriptor_set_layout());
}

DescriptorPool::DescriptorPool(vulkan::DescriptorPool descriptorPool) :
    m_descriptorPool(std::move(descriptorPool))
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
   const auto res = vkFreeDescriptorSets(m_descriptorPool.parent(), *m_descriptorPool, static_cast<u32>(sets.size()), sets.data());
   if (res != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }
   return Status::Success;
}

Result<DescriptorArray> DescriptorPool::allocate_array(const DescriptorLayoutArray& descriptorLayouts)
{
   VkDescriptorSetAllocateInfo descriptorSetsInfo{};
   descriptorSetsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   descriptorSetsInfo.descriptorPool = *m_descriptorPool;
   descriptorSetsInfo.descriptorSetCount = static_cast<u32>(descriptorLayouts.layouts().size());
   descriptorSetsInfo.pSetLayouts = descriptorLayouts.layouts().data();

   std::vector<VkDescriptorSet> descriptorSets{};
   descriptorSets.resize(descriptorLayouts.layouts().size());
   if (const auto res = vkAllocateDescriptorSets(m_descriptorPool.parent(), &descriptorSetsInfo, descriptorSets.data());
       res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return DescriptorArray(m_descriptorPool.parent(), *m_descriptorPool, std::move(descriptorSets));
}

}// namespace triglav::graphics_api