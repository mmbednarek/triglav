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

DescriptorPool::DescriptorPool(vulkan::DescriptorPool descriptor_pool) :
    m_descriptor_pool(std::move(descriptor_pool))
{
}

Status DescriptorPool::allocate_descriptors(const std::span<VkDescriptorSetLayout> in_layouts, std::span<VkDescriptorSet> out_sets)
{
   VkDescriptorSetAllocateInfo descriptor_sets_info{};
   descriptor_sets_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   descriptor_sets_info.descriptorPool = *m_descriptor_pool;
   descriptor_sets_info.descriptorSetCount = static_cast<u32>(in_layouts.size());
   descriptor_sets_info.pSetLayouts = in_layouts.data();

   if (const auto res = vkAllocateDescriptorSets(m_descriptor_pool.parent(), &descriptor_sets_info, out_sets.data()); res != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }

   return Status::Success;
}

Status DescriptorPool::free_descriptors(const std::span<VkDescriptorSet> sets)
{
   const auto res = vkFreeDescriptorSets(m_descriptor_pool.parent(), *m_descriptor_pool, static_cast<u32>(sets.size()), sets.data());
   if (res != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }
   return Status::Success;
}

Result<DescriptorArray> DescriptorPool::allocate_array(const DescriptorLayoutArray& descriptor_layouts)
{
   VkDescriptorSetAllocateInfo descriptor_sets_info{};
   descriptor_sets_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   descriptor_sets_info.descriptorPool = *m_descriptor_pool;
   descriptor_sets_info.descriptorSetCount = static_cast<u32>(descriptor_layouts.layouts().size());
   descriptor_sets_info.pSetLayouts = descriptor_layouts.layouts().data();

   std::vector<VkDescriptorSet> descriptor_sets{};
   descriptor_sets.resize(descriptor_layouts.layouts().size());
   if (const auto res = vkAllocateDescriptorSets(m_descriptor_pool.parent(), &descriptor_sets_info, descriptor_sets.data());
       res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return DescriptorArray(m_descriptor_pool.parent(), *m_descriptor_pool, std::move(descriptor_sets));
}

}// namespace triglav::graphics_api