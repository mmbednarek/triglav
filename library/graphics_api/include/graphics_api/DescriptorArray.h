#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "DescriptorView.h"

namespace graphics_api {

class DescriptorArray
{
 public:
   DescriptorArray(VkDevice device, VkDescriptorPool descriptorPool, VkPipelineLayout pipelineLayout, std::vector<VkDescriptorSet> descriptorSets);
   ~DescriptorArray();

   DescriptorArray(const DescriptorArray &other)            = delete;
   DescriptorArray &operator=(const DescriptorArray &other) = delete;
   DescriptorArray(DescriptorArray &&other) noexcept;
   DescriptorArray &operator=(DescriptorArray &&other) noexcept;

   [[nodiscard]] size_t count() const;
   [[nodiscard]] VkPipelineLayout pipeline_layout() const;
   [[nodiscard]] DescriptorView at(size_t index) const;
   [[nodiscard]] DescriptorView operator[](size_t index) const;

 private:
   VkDevice m_device;
   VkDescriptorPool m_descriptorPool;
   VkPipelineLayout m_pipelineLayout;
   std::vector<VkDescriptorSet> m_descriptorSets;
};

}// namespace graphics_api
