#pragma once

#include <vulkan/vulkan.h>

namespace triglav::graphics_api {

class DescriptorView
{
 public:
   DescriptorView(VkDescriptorSet descriptor_set);

   [[nodiscard]] VkDescriptorSet vulkan_descriptor_set() const;

 private:
   VkDescriptorSet m_descriptor_set{};
};

}// namespace triglav::graphics_api