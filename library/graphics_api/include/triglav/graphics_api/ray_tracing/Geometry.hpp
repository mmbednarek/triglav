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
   void add_triangles(const Buffer& vertexBuffer, const Buffer& indexBuffer, ColorFormat vertexFormat, MemorySize vertexSize,
                      u32 vertexCount, u32 triangleCount);
   void add_bounding_boxes(const Buffer& boundingBoxBuffer, MemorySize bbSize, u32 bbCount);
   void add_instances(const Buffer& instanceBuffer, u32 instanceCount);

   [[nodiscard]] VkAccelerationStructureBuildSizesInfoKHR size_requirement(Device& device) const;
   [[nodiscard]] VkAccelerationStructureBuildGeometryInfoKHR build(AccelerationStructure& dstAs, const BufferHeap::Section& scratchBuffer);
   [[nodiscard]] VkAccelerationStructureBuildRangeInfoKHR* ranges();
   void finalize(VkAccelerationStructureTypeKHR accelerationStructureType);

 private:
   AccelerationStructure* m_lastAccelerationStructure{};
   VkAccelerationStructureBuildGeometryInfoKHR m_buildInfo{};
   std::vector<u32> m_primitiveCounts{};
   std::vector<VkAccelerationStructureGeometryKHR> m_geometries;
   std::vector<VkAccelerationStructureBuildRangeInfoKHR> m_ranges;
};

class GeometryBuildContext
{
 public:
   GeometryBuildContext(Device& device, AccelerationStructurePool& asPool, BufferHeap& scratchBufferHeap);

   void add_triangle_buffer(const Buffer& vertexBuffer, const Buffer& indexBuffer, ColorFormat vertexFormat, MemorySize vertexSize,
                            u32 maxVertexCount, u32 triangleCount);
   void add_bounding_box_buffer(const Buffer& boundingBoxBuffer, MemorySize bbSize, u32 bbCount);
   void add_instance_buffer(const Buffer& instanceBuffer, u32 instanceCount);
   AccelerationStructure* commit_triangles();
   AccelerationStructure* commit_bounding_boxes();
   AccelerationStructure* commit_instances();

   void build_acceleration_structures(CommandList& cmdList);

 private:
   Device& m_device;
   AccelerationStructurePool& m_asPool;
   BufferHeap& m_scratchBufferHeap;

   GeometryBuildInfo m_currentTriangles;
   GeometryBuildInfo m_currentBoundingBoxes;
   GeometryBuildInfo m_currentInstances;
   std::vector<GeometryBuildInfo> m_triangleGeometries;
   std::vector<GeometryBuildInfo> m_boundingBoxesGeometries;
   std::vector<GeometryBuildInfo> m_instanceGeometries;
   std::vector<VkAccelerationStructureBuildGeometryInfoKHR> m_buildInfos;
   std::vector<VkAccelerationStructureBuildRangeInfoKHR*> m_buildRangePtrs;
};

/*
AccelerationStructureBuildContext ctx;
ctx.add_triangle_buffer(...);
ctx.add_triangle_buffer(...);
ctx.build_bottom_level_triangles();
 */

}// namespace triglav::graphics_api::ray_tracing
