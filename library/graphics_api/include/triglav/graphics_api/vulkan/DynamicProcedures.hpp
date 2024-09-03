#pragma once

#include <utility>
#include <vulkan/vulkan.h>

#define TRIGLAV_GAPI_VULKAN_DYNAMIC_PROCS            \
   TRIGLAV_HANDLE(vkCreateAccelerationStructureKHR)  \
   TRIGLAV_HANDLE(vkDestroyAccelerationStructureKHR) \
   TRIGLAV_HANDLE(vkCmdPushDescriptorSetKHR)

namespace triglav::graphics_api::vulkan {

class DynamicProcedures
{
 public:
   void init(VkDevice vulkanDevice);

   [[nodiscard]] static DynamicProcedures& the();

#define TRIGLAV_HANDLE(proc)               \
   [[nodiscard]] PFN_##proc& proc_##proc() \
   {                                       \
      return m_##proc;                     \
   }
   TRIGLAV_GAPI_VULKAN_DYNAMIC_PROCS
#undef TRIGLAV_HANDLE


 private:
#define TRIGLAV_HANDLE(proc) PFN_##proc m_##proc;
   TRIGLAV_GAPI_VULKAN_DYNAMIC_PROCS
#undef TRIGLAV_HANDLE
};

VkResult vkCreateAccelerationStructureKHR(VkDevice device, const VkAccelerationStructureCreateInfoKHR* pCreateInfo,
                                          const VkAllocationCallbacks* pAllocator, VkAccelerationStructureKHR* pAccelerationStructure);

void vkDestroyAccelerationStructureKHR(VkDevice device, VkAccelerationStructureKHR accelerationStructure,
                                       const VkAllocationCallbacks* pAllocator);

void vkCmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set,
                               uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites);

}// namespace triglav::graphics_api::vulkan