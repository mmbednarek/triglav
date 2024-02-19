#pragma once

#include <vulkan/vulkan.h>

namespace triglav::graphics_api {

class DescriptorView
{
 public:
   DescriptorView(VkDescriptorSet descriptorSet, VkPipelineLayout pipelineLayout);

   [[nodiscard]] VkDescriptorSet vulkan_descriptor_set() const;
   [[nodiscard]] VkPipelineLayout vulkan_pipeline_layout() const;

 private:
   VkDescriptorSet m_descriptorSet{};
   VkPipelineLayout m_pipelineLayout{};
};

}// namespace graphics_api