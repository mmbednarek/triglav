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

class BottomLevelGeometry
{
 public:
    void add_triangles(void* vertexData, void* indexData, ColorFormat vertexFormat, MemorySize vertexSize, u32 maxVertexCount, u32 triangleCount);
    void add_bounding_boxes(void* boundingBoxes, MemorySize bbSize, u32 bbCount);

    [[nodiscard]] MemorySize size_requirement() const;
    [[nodiscard]] VkAccelerationStructureBuildGeometryInfoKHR build(AccelerationStructure* srcAs, AccelerationStructure& dstAs);
   [[nodiscard]] AccelerationStructure* update_last_acceleration_structure(AccelerationStructure* as);

 private:
   AccelerationStructure* m_lastAccelerationStructure{};
   MemorySize m_sizeRequirement{};
   std::vector<VkAccelerationStructureGeometryKHR> m_geometries;
   std::vector<VkAccelerationStructureBuildRangeInfoKHR> m_ranges;
};

class GeometryBuildContext
{
 public:
   explicit GeometryBuildContext(AccelerationStructurePool& asPool);

   void add_triangle_buffer(void* vertexData, void* indexData, ColorFormat vertexFormat, MemorySize vertexSize, u32 maxVertexCount, u32 triangleCount);
   void commit_triangles();

   void build_acceleration_structures(CommandList& cmdList);
 private:
   AccelerationStructurePool& m_asPool;

   BottomLevelGeometry m_currentTriangles;
   BottomLevelGeometry m_currentBoundingBoxes;
   std::vector<BottomLevelGeometry> m_triangleGeometries;
   std::vector<BottomLevelGeometry> m_boundingBoxesGeometries;
   std::vector<VkAccelerationStructureBuildGeometryInfoKHR> m_buildInfos;
   std::vector<VkAccelerationStructureBuildRangeInfoKHR*> m_buildRangePtrs;
};

/*
AccelerationStructureBuildContext ctx;
ctx.add_triangle_buffer(...);
ctx.add_triangle_buffer(...);
ctx.build_bottom_level_triangles();
 */

}
