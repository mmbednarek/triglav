#pragma once

#include "vulkan/ObjectWrapper.hpp"

#include "DescriptorArray.hpp"
#include "GraphicsApi.hpp"

#include <span>

namespace triglav::graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(DescriptorPool, Device)
DECLARE_VLK_WRAPPED_CHILD_OBJECT(DescriptorSetLayout, Device)

class DescriptorPool
{
 public:
   DescriptorPool(vulkan::DescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout);

   [[nodiscard]] Status allocate_descriptors(std::span<VkDescriptorSetLayout> inLayouts, std::span<VkDescriptorSet> outSets);
   [[nodiscard]] Status free_descriptors(std::span<VkDescriptorSet> sets);

   [[nodiscard]] Result<DescriptorArray> allocate_array(size_t descriptorCount);

 private:
   vulkan::DescriptorPool m_descriptorPool;
   VkDescriptorSetLayout m_descriptorSetLayout;
};

}// namespace triglav::graphics_api