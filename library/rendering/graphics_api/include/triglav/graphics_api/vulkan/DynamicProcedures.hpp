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
   TRIGLAV_HANDLE(vkCmdPushDescriptorSetKHR)                  \
   TRIGLAV_HANDLE(vkSetDebugUtilsObjectNameEXT)


namespace triglav::graphics_api::vulkan {

class DynamicProcedures
{
 public:
   void init(VkDevice vulkan_device);

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

VkResult vkCreateAccelerationStructureKHR(VkDevice device, const VkAccelerationStructureCreateInfoKHR* p_create_info,
                                          const VkAllocationCallbacks* p_allocator, VkAccelerationStructureKHR* p_acceleration_structure);

void vkDestroyAccelerationStructureKHR(VkDevice device, VkAccelerationStructureKHR acceleration_structure,
                                       const VkAllocationCallbacks* p_allocator);

void vkCmdPushDescriptorSetKHR(VkCommandBuffer command_buffer, VkPipelineBindPoint pipeline_bind_point, VkPipelineLayout layout,
                               uint32_t set, uint32_t descriptor_write_count, const VkWriteDescriptorSet* p_descriptor_writes);

void vkCmdBuildAccelerationStructuresKHR(VkCommandBuffer command_buffer, uint32_t info_count,
                                         const VkAccelerationStructureBuildGeometryInfoKHR* p_infos,
                                         const VkAccelerationStructureBuildRangeInfoKHR* const* pp_build_range_infos);

void vkGetAccelerationStructureBuildSizesKHR(VkDevice device, VkAccelerationStructureBuildTypeKHR build_type,
                                             const VkAccelerationStructureBuildGeometryInfoKHR* p_build_info,
                                             const uint32_t* p_max_primitive_counts, VkAccelerationStructureBuildSizesInfoKHR* p_size_info);

VkDeviceAddress vkGetAccelerationStructureDeviceAddressKHR(VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR* p_info);

VkResult vkCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferred_operation, VkPipelineCache pipeline_cache,
                                        uint32_t create_info_count, const VkRayTracingPipelineCreateInfoKHR* p_create_infos,
                                        const VkAllocationCallbacks* p_allocator, VkPipeline* p_pipelines);

VkResult vkGetRayTracingShaderGroupHandlesKHR(VkDevice device, VkPipeline pipeline, uint32_t first_group, uint32_t group_count,
                                              size_t data_size, void* p_data);
void vkCmdTraceRaysKHR(VkCommandBuffer command_buffer, const VkStridedDeviceAddressRegionKHR* p_raygen_shader_binding_table,
                       const VkStridedDeviceAddressRegionKHR* p_miss_shader_binding_table,
                       const VkStridedDeviceAddressRegionKHR* p_hit_shader_binding_table,
                       const VkStridedDeviceAddressRegionKHR* p_callable_shader_binding_table, uint32_t width, uint32_t height,
                       uint32_t depth);

void vkCmdDrawIndexedIndirectCount(VkCommandBuffer command_buffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer count_buffer,
                                   VkDeviceSize count_buffer_offset, uint32_t max_draw_count, uint32_t stride);

VkResult vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* p_name_info);

}// namespace triglav::graphics_api::vulkan