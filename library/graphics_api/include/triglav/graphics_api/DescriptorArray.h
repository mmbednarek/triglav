#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "DescriptorView.h"

namespace triglav::graphics_api {

class DescriptorArray
{
 public:
   DescriptorArray(VkDevice device, VkDescriptorPool descriptorPool, std::vector<VkDescriptorSet> descriptorSets);
   ~DescriptorArray();

   DescriptorArray(const DescriptorArray& other) = delete;
   DescriptorArray& operator=(const DescriptorArray& other) = delete;
   DescriptorArray(DescriptorArray&& other) noexcept;
   DescriptorArray& operator=(DescriptorArray&& other) noexcept;

   [[nodiscard]] size_t count() const;
   [[nodiscard]] DescriptorView at(size_t index) const;
   [[nodiscard]] DescriptorView operator[](size_t index) const;

 private:
   VkDevice m_device;
   VkDescriptorPool m_descriptorPool;
   std::vector<VkDescriptorSet> m_descriptorSets;
};

}// namespace triglav::graphics_api
