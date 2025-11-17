#include "ray_tracing/Geometry.hpp"
#include "../vulkan/Util.hpp"
#include "CommandList.hpp"
#include "Device.hpp"

namespace triglav::graphics_api::ray_tracing {

void GeometryBuildInfo::add_triangles(const Buffer& vertex_buffer, const Buffer& index_buffer, ColorFormat vertex_format,
                                      MemorySize vertex_size, u32 vertex_count, u32 triangle_count)
{
   VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
   geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
   geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
   geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
   geometry.geometry.triangles.pNext = nullptr;
   geometry.geometry.triangles.vertexData.deviceAddress = vertex_buffer.vulkan_device_address();
   geometry.geometry.triangles.indexData.deviceAddress = index_buffer.vulkan_device_address();
   geometry.geometry.triangles.vertexStride = vertex_size;
   geometry.geometry.triangles.vertexFormat = graphics_api::vulkan::to_vulkan_color_format(vertex_format).value_or(VK_FORMAT_MAX_ENUM);
   geometry.geometry.triangles.maxVertex = vertex_count - 1;
   geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
   geometry.geometry.triangles.transformData.deviceAddress = 0;
   m_geometries.push_back(geometry);

   VkAccelerationStructureBuildRangeInfoKHR ranges{};
   ranges.primitiveCount = triangle_count;
   m_ranges.push_back(ranges);

   m_primitive_counts.push_back(triangle_count);
}

void GeometryBuildInfo::add_bounding_boxes(const Buffer& bounding_box_buffer, MemorySize bb_size, u32 bb_count)
{
   VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
   geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
   geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
   geometry.geometry.aabbs.pNext = nullptr;
   geometry.geometry.aabbs.data.deviceAddress = bounding_box_buffer.vulkan_device_address();
   geometry.geometry.aabbs.stride = bb_size;
   m_geometries.push_back(geometry);

   VkAccelerationStructureBuildRangeInfoKHR ranges{};
   ranges.primitiveCount = bb_count;
   m_ranges.push_back(ranges);

   m_primitive_counts.push_back(bb_count);
}

void GeometryBuildInfo::add_instances(const Buffer& instance_buffer, u32 instance_count)
{
   VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
   geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
   geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
   geometry.geometry.instances.pNext = nullptr;
   geometry.geometry.instances.arrayOfPointers = false;
   geometry.geometry.instances.data.deviceAddress = instance_buffer.vulkan_device_address();
   m_geometries.push_back(geometry);

   VkAccelerationStructureBuildRangeInfoKHR ranges{};
   ranges.primitiveCount = instance_count;
   m_ranges.push_back(ranges);

   m_primitive_counts.push_back(instance_count);
}

VkAccelerationStructureBuildSizesInfoKHR GeometryBuildInfo::size_requirement(Device& device) const
{
   VkAccelerationStructureBuildSizesInfoKHR out_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
   vulkan::vkGetAccelerationStructureBuildSizesKHR(device.vulkan_device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &m_build_info,
                                                   m_primitive_counts.data(), &out_info);

   return out_info;
}

VkAccelerationStructureBuildGeometryInfoKHR GeometryBuildInfo::build(AccelerationStructure& dst_as,
                                                                     const BufferHeap::Section& scratch_buffer)
{
   m_build_info.dstAccelerationStructure = dst_as.vulkan_acceleration_structure();
   m_build_info.scratchData.deviceAddress = scratch_buffer.buffer->vulkan_device_address() + scratch_buffer.offset;
   m_last_acceleration_structure = &dst_as;
   return m_build_info;
}

VkAccelerationStructureBuildRangeInfoKHR* GeometryBuildInfo::ranges()
{
   return m_ranges.data();
}
void GeometryBuildInfo::finalize(VkAccelerationStructureTypeKHR acceleration_structure_type)
{
   m_build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
   m_build_info.pNext = nullptr;
   m_build_info.type = acceleration_structure_type;
   m_build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
   m_build_info.srcAccelerationStructure =
      m_last_acceleration_structure == nullptr ? nullptr : m_last_acceleration_structure->vulkan_acceleration_structure();
   m_build_info.mode = m_last_acceleration_structure == nullptr ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR
                                                                : VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
   m_build_info.dstAccelerationStructure = nullptr;
   m_build_info.geometryCount = static_cast<u32>(m_geometries.size());
   m_build_info.pGeometries = m_geometries.data();
   m_build_info.scratchData.deviceAddress = 0;
}

// *********************************************
// ****** GEOMETRY BUILD CONTEXT ***************
// *********************************************

GeometryBuildContext::GeometryBuildContext(Device& device, AccelerationStructurePool& as_pool, BufferHeap& scratch_buffer_heap) :
    m_device(device),
    m_as_pool(as_pool),
    m_scratch_buffer_heap(scratch_buffer_heap)
{
}

void GeometryBuildContext::add_triangle_buffer(const Buffer& vertex_buffer, const Buffer& index_buffer, ColorFormat vertex_format,
                                               const MemorySize vertex_size, const u32 max_vertex_count, const u32 triangle_count)
{
   m_current_triangles.add_triangles(vertex_buffer, index_buffer, vertex_format, vertex_size, max_vertex_count, triangle_count);
}

void GeometryBuildContext::add_bounding_box_buffer(const Buffer& bounding_box_buffer, const MemorySize bb_size, const u32 bb_count)
{
   m_current_bounding_boxes.add_bounding_boxes(bounding_box_buffer, bb_size, bb_count);
}

void GeometryBuildContext::add_instance_buffer(const Buffer& instance_buffer, const u32 instance_count)
{
   m_current_instances.add_instances(instance_buffer, instance_count);
}

AccelerationStructure* GeometryBuildContext::commit_triangles()
{
   auto& geo = m_triangle_geometries.emplace_back(std::move(m_current_triangles));

   geo.finalize(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);
   const auto requirement = geo.size_requirement(m_device);

   auto* next_acceleration_struct =
      m_as_pool.acquire_acceleration_structure(AccelerationStructureType::BottomLevel, requirement.accelerationStructureSize);
   auto scratch_section = m_scratch_buffer_heap.allocate_section(requirement.buildScratchSize);

   m_build_infos.push_back(geo.build(*next_acceleration_struct, scratch_section));
   m_build_range_ptrs.push_back(geo.ranges());

   return next_acceleration_struct;
}

AccelerationStructure* GeometryBuildContext::commit_bounding_boxes()
{
   auto& geo = m_bounding_boxes_geometries.emplace_back(std::move(m_current_bounding_boxes));

   geo.finalize(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);
   const auto requirement = geo.size_requirement(m_device);

   auto* next_acceleration_struct =
      m_as_pool.acquire_acceleration_structure(AccelerationStructureType::BottomLevel, requirement.accelerationStructureSize);
   auto scratch_section = m_scratch_buffer_heap.allocate_section(requirement.buildScratchSize);

   m_build_infos.push_back(geo.build(*next_acceleration_struct, scratch_section));
   m_build_range_ptrs.push_back(geo.ranges());

   return next_acceleration_struct;
}

AccelerationStructure* GeometryBuildContext::commit_instances()
{
   auto& geo = m_instance_geometries.emplace_back(std::move(m_current_instances));

   geo.finalize(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);
   const auto requirement = geo.size_requirement(m_device);

   auto* next_acceleration_struct =
      m_as_pool.acquire_acceleration_structure(AccelerationStructureType::TopLevel, requirement.accelerationStructureSize);
   auto scratch_section = m_scratch_buffer_heap.allocate_section(requirement.buildScratchSize);

   m_build_infos.push_back(geo.build(*next_acceleration_struct, scratch_section));
   m_build_range_ptrs.push_back(geo.ranges());

   return next_acceleration_struct;
}

void GeometryBuildContext::build_acceleration_structures(CommandList& cmd_list)
{
   vulkan::vkCmdBuildAccelerationStructuresKHR(cmd_list.vulkan_command_buffer(), static_cast<u32>(m_build_infos.size()),
                                               m_build_infos.data(), m_build_range_ptrs.data());
}

}// namespace triglav::graphics_api::ray_tracing
