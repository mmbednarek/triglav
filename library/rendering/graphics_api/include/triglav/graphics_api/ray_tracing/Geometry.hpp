#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "../GraphicsApi.hpp"
#include "AccelerationStructure.hpp"
#include "AccelerationStructurePool.hpp"


namespace triglav::graphics_api {
class CommandList;
}
namespace triglav::graphics_api::ray_tracing {

class GeometryBuildInfo
{
 public:
   void add_triangles(const Buffer& vertex_buffer, const Buffer& index_buffer, ColorFormat vertex_format, MemorySize vertex_size,
                      u32 vertex_count, u32 triangle_count);
   void add_bounding_boxes(const Buffer& bounding_box_buffer, MemorySize bb_size, u32 bb_count);
   void add_instances(const Buffer& instance_buffer, u32 instance_count);

   [[nodiscard]] VkAccelerationStructureBuildSizesInfoKHR size_requirement(Device& device) const;
   [[nodiscard]] VkAccelerationStructureBuildGeometryInfoKHR build(AccelerationStructure& dst_as,
                                                                   const BufferHeap::Section& scratch_buffer);
   [[nodiscard]] VkAccelerationStructureBuildRangeInfoKHR* ranges();
   void finalize(VkAccelerationStructureTypeKHR acceleration_structure_type);

 private:
   AccelerationStructure* m_last_acceleration_structure{};
   VkAccelerationStructureBuildGeometryInfoKHR m_build_info{};
   std::vector<u32> m_primitive_counts{};
   std::vector<VkAccelerationStructureGeometryKHR> m_geometries;
   std::vector<VkAccelerationStructureBuildRangeInfoKHR> m_ranges;
};

class GeometryBuildContext
{
 public:
   GeometryBuildContext(Device& device, AccelerationStructurePool& as_pool, BufferHeap& scratch_buffer_heap);

   void add_triangle_buffer(const Buffer& vertex_buffer, const Buffer& index_buffer, ColorFormat vertex_format, MemorySize vertex_size,
                            u32 max_vertex_count, u32 triangle_count);
   void add_bounding_box_buffer(const Buffer& bounding_box_buffer, MemorySize bb_size, u32 bb_count);
   void add_instance_buffer(const Buffer& instance_buffer, u32 instance_count);
   AccelerationStructure* commit_triangles();
   AccelerationStructure* commit_bounding_boxes();
   AccelerationStructure* commit_instances();

   void build_acceleration_structures(CommandList& cmd_list);

 private:
   Device& m_device;
   AccelerationStructurePool& m_as_pool;
   BufferHeap& m_scratch_buffer_heap;

   GeometryBuildInfo m_current_triangles;
   GeometryBuildInfo m_current_bounding_boxes;
   GeometryBuildInfo m_current_instances;
   std::vector<GeometryBuildInfo> m_triangle_geometries;
   std::vector<GeometryBuildInfo> m_bounding_boxes_geometries;
   std::vector<GeometryBuildInfo> m_instance_geometries;
   std::vector<VkAccelerationStructureBuildGeometryInfoKHR> m_build_infos;
   std::vector<VkAccelerationStructureBuildRangeInfoKHR*> m_build_range_ptrs;
};

/*
AccelerationStructureBuildContext ctx;
ctx.add_triangle_buffer(...);
ctx.add_triangle_buffer(...);
ctx.build_bottom_level_triangles();
 */

}// namespace triglav::graphics_api::ray_tracing
