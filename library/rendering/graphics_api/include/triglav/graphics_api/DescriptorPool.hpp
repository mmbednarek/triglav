#pragma once

#include "vulkan/ObjectWrapper.hpp"

#include "DescriptorArray.hpp"
#include "GraphicsApi.hpp"

#include <span>

namespace triglav::graphics_api {

class Pipeline;
class DescriptorLayoutCache;

DECLARE_VLK_WRAPPED_CHILD_OBJECT(DescriptorPool, Device)
DECLARE_VLK_WRAPPED_CHILD_OBJECT(DescriptorSetLayout, Device)

class DescriptorLayoutArray
{
 public:
   void add_from_cache(DescriptorLayoutCache& cache, std::span<DescriptorBinding> bindings);
   void add_from_pipeline(const Pipeline& pipeline);

   const std::vector<VkDescriptorSetLayout>& layouts() const;

 private:
   std::vector<VkDescriptorSetLayout> m_layouts;
};

class DescriptorPool
{
 public:
   explicit DescriptorPool(vulkan::DescriptorPool descriptor_pool);

   [[nodiscard]] Status allocate_descriptors(std::span<VkDescriptorSetLayout> in_layouts, std::span<VkDescriptorSet> out_sets);
   [[nodiscard]] Status free_descriptors(std::span<VkDescriptorSet> sets);

   [[nodiscard]] Result<DescriptorArray> allocate_array(const DescriptorLayoutArray& descriptor_layouts);

 private:
   vulkan::DescriptorPool m_descriptor_pool;
};

}// namespace triglav::graphics_api