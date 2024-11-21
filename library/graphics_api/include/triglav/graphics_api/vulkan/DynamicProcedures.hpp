#pragma once

#include <utility>
#include <vulkan/vulkan.h>

#define TRIGLAV_GAPI_VULKAN_DYNAMIC_PROCS                     \
   TRIGLAV_HANDLE(vkCreateAccelerationStructureKHR)           \
   TRIGLAV_HANDLE(vkDestroyAccelerationStructureKHR)          \
   TRIGLAV_HANDLE(vkCmdBuildAccelerationStructuresKHR)        \
   TRIGLAV_HANDLE(vkGetAccelerationStructureBuildSizesKHR)    \
   TRIGLAV_HANDLE(vkGetAccelerationStructureDeviceAddressKHR) \
   TRIGLAV_HANDLE(vkCreateRayTracingPipelinesKHR)             \
   TRIGLAV_HANDLE(vkGetRayTracingShaderGroupHandlesKHR)       \
   TRIGLAV_HANDLE(vkCmdDrawIndexedIndirectCount)              \
   TRIGLAV_HANDLE(vkCmdTraceRaysKHR)                          \
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

void vkCmdBuildAccelerationStructuresKHR(VkCommandBuffer commandBuffer, uint32_t infoCount,
                                         const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
                                         const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos);

void vkGetAccelerationStructureBuildSizesKHR(VkDevice device, VkAccelerationStructureBuildTypeKHR buildType,
                                             const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
                                             const uint32_t* pMaxPrimitiveCounts, VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo);

VkDeviceAddress vkGetAccelerationStructureDeviceAddressKHR(VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR* pInfo);

VkResult vkCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache,
                                        uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
                                        const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines);

VkResult vkGetRayTracingShaderGroupHandlesKHR(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount,
                                              size_t dataSize, void* pData);
void vkCmdTraceRaysKHR(VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable,
                       const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable,
                       const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable,
                       const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable, uint32_t width, uint32_t height, uint32_t depth);

void vkCmdDrawIndexedIndirectCount(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer,
                                   VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride);

}// namespace triglav::graphics_api::vulkan