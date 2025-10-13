#pragma once

#include <vulkan/vulkan.h>

namespace triglav::graphics_api {

class DescriptorView
{
 public:
   DescriptorView(VkDescriptorSet descriptorSet);

   [[nodiscard]] VkDescriptorSet vulkan_descriptor_set() const;

 private:
   VkDescriptorSet m_descriptorSet{};
};

}// namespace triglav::graphics_api