#include "vulkan/DynamicProcedures.hpp"

namespace triglav::graphics_api::vulkan {

void DynamicProcedures::init(VkDevice vulkanDevice){
#define TRIGLAV_HANDLE(proc) m_##proc = reinterpret_cast<PFN_##proc>(vkGetDeviceProcAddr(vulkanDevice, #proc));
   TRIGLAV_GAPI_VULKAN_DYNAMIC_PROCS
#undef TRIGLAV_HANDLE
}

DynamicProcedures& DynamicProcedures::the()
{
   static DynamicProcedures instance;
   return instance;
}

VkResult vkCreateAccelerationStructureKHR(VkDevice device, const VkAccelerationStructureCreateInfoKHR* pCreateInfo,
                                          const VkAllocationCallbacks* pAllocator, VkAccelerationStructureKHR* pAccelerationStructure)
{
   return DynamicProcedures::the().proc_vkCreateAccelerationStructureKHR()(device, pCreateInfo, pAllocator, pAccelerationStructure);
}

void vkDestroyAccelerationStructureKHR(VkDevice device, VkAccelerationStructureKHR accelerationStructure,
                                       const VkAllocationCallbacks* pAllocator)
{
   DynamicProcedures::the().proc_vkDestroyAccelerationStructureKHR()(device, accelerationStructure, pAllocator);
}

void vkCmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set,
                               uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites)
{
   DynamicProcedures::the().proc_vkCmdPushDescriptorSetKHR()(commandBuffer, pipelineBindPoint, layout, set, descriptorWriteCount,
                                                             pDescriptorWrites);
}

}// namespace triglav::graphics_api::vulkan
