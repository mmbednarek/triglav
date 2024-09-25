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

void vkCmdBuildAccelerationStructuresKHR(VkCommandBuffer commandBuffer, uint32_t infoCount,
                                         const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
                                         const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos)
{
   DynamicProcedures::the().proc_vkCmdBuildAccelerationStructuresKHR()(commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
}

void vkGetAccelerationStructureBuildSizesKHR(VkDevice device, VkAccelerationStructureBuildTypeKHR buildType,
                                             const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
                                             const uint32_t* pMaxPrimitiveCounts, VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo)
{
   DynamicProcedures::the().proc_vkGetAccelerationStructureBuildSizesKHR()(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
}

VkDeviceAddress vkGetAccelerationStructureDeviceAddressKHR(VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR* pInfo)
{
   return DynamicProcedures::the().proc_vkGetAccelerationStructureDeviceAddressKHR()(device, pInfo);
}

VkResult vkCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache,
                                        uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
                                        const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
   return DynamicProcedures::the().proc_vkCreateRayTracingPipelinesKHR()(device, deferredOperation, pipelineCache, createInfoCount,
                                                                         pCreateInfos, pAllocator, pPipelines);
}

VkResult vkGetRayTracingShaderGroupHandlesKHR(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount,
                                              size_t dataSize, void* pData)
{
   return DynamicProcedures::the().proc_vkGetRayTracingShaderGroupHandlesKHR()(device, pipeline, firstGroup, groupCount, dataSize, pData);
}

void vkCmdTraceRaysKHR(VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable,
                       const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable,
                       const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable,
                       const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable, uint32_t width, uint32_t height, uint32_t depth)
{
   return DynamicProcedures::the().proc_vkCmdTraceRaysKHR()(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, width, height, depth);
}

}// namespace triglav::graphics_api::vulkan
