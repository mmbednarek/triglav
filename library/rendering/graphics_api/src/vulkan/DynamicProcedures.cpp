#include "vulkan/DynamicProcedures.hpp"

#include <mutex>

namespace triglav::graphics_api::vulkan {

void DynamicProcedures::init(VkDevice vulkan_device)
{
#define TRIGLAV_HANDLE(proc) m_##proc = reinterpret_cast<PFN_##proc>(vkGetDeviceProcAddr(vulkan_device, #proc));
   TRIGLAV_GAPI_VULKAN_DYNAMIC_PROCS
#undef TRIGLAV_HANDLE
}

DynamicProcedures& DynamicProcedures::the()
{
   static DynamicProcedures instance;
   return instance;
}

VkResult vkCreateAccelerationStructureKHR(VkDevice device, const VkAccelerationStructureCreateInfoKHR* p_create_info,
                                          const VkAllocationCallbacks* p_allocator, VkAccelerationStructureKHR* p_acceleration_structure)
{
   return DynamicProcedures::the().proc_vkCreateAccelerationStructureKHR()(device, p_create_info, p_allocator, p_acceleration_structure);
}

void vkDestroyAccelerationStructureKHR(VkDevice device, VkAccelerationStructureKHR acceleration_structure,
                                       const VkAllocationCallbacks* p_allocator)
{
   DynamicProcedures::the().proc_vkDestroyAccelerationStructureKHR()(device, acceleration_structure, p_allocator);
}

void vkCmdPushDescriptorSetKHR(VkCommandBuffer command_buffer, VkPipelineBindPoint pipeline_bind_point, VkPipelineLayout layout,
                               uint32_t set, uint32_t descriptor_write_count, const VkWriteDescriptorSet* p_descriptor_writes)
{
   DynamicProcedures::the().proc_vkCmdPushDescriptorSetKHR()(command_buffer, pipeline_bind_point, layout, set, descriptor_write_count,
                                                             p_descriptor_writes);
}

void vkCmdBuildAccelerationStructuresKHR(VkCommandBuffer command_buffer, uint32_t info_count,
                                         const VkAccelerationStructureBuildGeometryInfoKHR* p_infos,
                                         const VkAccelerationStructureBuildRangeInfoKHR* const* pp_build_range_infos)
{
   DynamicProcedures::the().proc_vkCmdBuildAccelerationStructuresKHR()(command_buffer, info_count, p_infos, pp_build_range_infos);
}

void vkGetAccelerationStructureBuildSizesKHR(VkDevice device, VkAccelerationStructureBuildTypeKHR build_type,
                                             const VkAccelerationStructureBuildGeometryInfoKHR* p_build_info,
                                             const uint32_t* p_max_primitive_counts, VkAccelerationStructureBuildSizesInfoKHR* p_size_info)
{
   DynamicProcedures::the().proc_vkGetAccelerationStructureBuildSizesKHR()(device, build_type, p_build_info, p_max_primitive_counts,
                                                                           p_size_info);
}

VkDeviceAddress vkGetAccelerationStructureDeviceAddressKHR(VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR* p_info)
{
   return DynamicProcedures::the().proc_vkGetAccelerationStructureDeviceAddressKHR()(device, p_info);
}

VkResult vkCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferred_operation, VkPipelineCache pipeline_cache,
                                        uint32_t create_info_count, const VkRayTracingPipelineCreateInfoKHR* p_create_infos,
                                        const VkAllocationCallbacks* p_allocator, VkPipeline* p_pipelines)
{
   return DynamicProcedures::the().proc_vkCreateRayTracingPipelinesKHR()(device, deferred_operation, pipeline_cache, create_info_count,
                                                                         p_create_infos, p_allocator, p_pipelines);
}

VkResult vkGetRayTracingShaderGroupHandlesKHR(VkDevice device, VkPipeline pipeline, uint32_t first_group, uint32_t group_count,
                                              size_t data_size, void* p_data)
{
   return DynamicProcedures::the().proc_vkGetRayTracingShaderGroupHandlesKHR()(device, pipeline, first_group, group_count, data_size,
                                                                               p_data);
}

void vkCmdTraceRaysKHR(VkCommandBuffer command_buffer, const VkStridedDeviceAddressRegionKHR* p_raygen_shader_binding_table,
                       const VkStridedDeviceAddressRegionKHR* p_miss_shader_binding_table,
                       const VkStridedDeviceAddressRegionKHR* p_hit_shader_binding_table,
                       const VkStridedDeviceAddressRegionKHR* p_callable_shader_binding_table, uint32_t width, uint32_t height,
                       uint32_t depth)
{
   return DynamicProcedures::the().proc_vkCmdTraceRaysKHR()(command_buffer, p_raygen_shader_binding_table, p_miss_shader_binding_table,
                                                            p_hit_shader_binding_table, p_callable_shader_binding_table, width, height,
                                                            depth);
}

void vkCmdDrawIndexedIndirectCount(const VkCommandBuffer command_buffer, const VkBuffer buffer, const VkDeviceSize offset,
                                   const VkBuffer count_buffer, const VkDeviceSize count_buffer_offset, const uint32_t max_draw_count,
                                   const uint32_t stride)
{
   return DynamicProcedures::the().proc_vkCmdDrawIndexedIndirectCount()(command_buffer, buffer, offset, count_buffer, count_buffer_offset,
                                                                        max_draw_count, stride);
}

VkResult vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* p_name_info)
{
   return DynamicProcedures::the().proc_vkSetDebugUtilsObjectNameEXT()(device, p_name_info);
}

}// namespace triglav::graphics_api::vulkan
