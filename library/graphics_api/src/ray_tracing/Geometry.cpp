#include "ray_tracing/Geometry.hpp"
#include "../vulkan/Util.hpp"

namespace triglav::graphics_api::ray_tracing {

void BottomLevelGeometry::add_triangles(void* vertexData, void* indexData, ColorFormat vertexFormat, MemorySize vertexSize,
                                        u32 maxVertexCount, u32 triangleCount)
{
   VkAccelerationStructureGeometryKHR geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
   geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
   geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
   geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
   geometry.geometry.triangles.pNext = nullptr;
   geometry.geometry.triangles.vertexData.hostAddress = vertexData;
   geometry.geometry.triangles.indexData.hostAddress = indexData;
   geometry.geometry.triangles.vertexStride = vertexSize;
   geometry.geometry.triangles.vertexFormat = graphics_api::vulkan::to_vulkan_color_format(vertexFormat).value_or(VK_FORMAT_MAX_ENUM);
   geometry.geometry.triangles.maxVertex = maxVertexCount;
   geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
   m_geometries.push_back(geometry);

   VkAccelerationStructureBuildRangeInfoKHR ranges{};
   ranges.primitiveCount = triangleCount;
   m_ranges.push_back(ranges);

   m_sizeRequirement += triangleCount * vertexSize;
}

void BottomLevelGeometry::add_bounding_boxes(void* boundingBoxes, MemorySize bbSize, u32 bbCount)
{
   VkAccelerationStructureGeometryKHR geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
   geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
   geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
   geometry.geometry.aabbs.pNext = nullptr;
   geometry.geometry.aabbs.data.hostAddress = boundingBoxes;
   geometry.geometry.aabbs.stride = bbSize;
   m_geometries.push_back(geometry);

   VkAccelerationStructureBuildRangeInfoKHR ranges{};
   ranges.primitiveCount = bbCount;
   m_ranges.push_back(ranges);
}

MemorySize BottomLevelGeometry::size_requirement() const
{
   return m_sizeRequirement;
}

VkAccelerationStructureBuildGeometryInfoKHR BottomLevelGeometry::build(AccelerationStructure* srcAs, AccelerationStructure& dstAs)
{
   VkAccelerationStructureBuildGeometryInfoKHR result{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
   result.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
   result.srcAccelerationStructure = srcAs == nullptr ? nullptr : *srcAs->vulkan_acceleration_structure();
   result.mode = srcAs == nullptr ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
   result.dstAccelerationStructure = *dstAs.vulkan_acceleration_structure();
   result.geometryCount = m_geometries.size();
   result.pGeometries = m_geometries.data();
   return result;
}
AccelerationStructure* BottomLevelGeometry::update_last_acceleration_structure(AccelerationStructure* as)
{
   return std::exchange(m_lastAccelerationStructure, as);
}

// *********************************************
// ****** GEOMETRY BUILD CONTEXT ***************
// *********************************************

GeometryBuildContext::GeometryBuildContext(AccelerationStructurePool& asPool) :
   m_asPool(asPool)
{
}

void GeometryBuildContext::add_triangle_buffer(void* vertexData, void* indexData, ColorFormat vertexFormat, MemorySize vertexSize,
                                               u32 maxVertexCount, u32 triangleCount)
{
   m_currentTriangles.add_triangles(vertexData, indexData, vertexFormat, vertexSize, maxVertexCount, triangleCount);
}

void GeometryBuildContext::commit_triangles()
{
   auto& geo = m_triangleGeometries.emplace_back(std::move(m_currentTriangles));

   auto* nextAc = m_asPool.acquire_acceleration_structure(geo.size_requirement());
   auto lastAc = geo.update_last_acceleration_structure(nextAc);
   auto buildInfo = geo.build(lastAc, *nextAc);

   m_buildInfos.push_back(buildInfo);
}

void GeometryBuildContext::build_acceleration_structures(CommandList& cmdList)
{
   vkCmdBuildAccelerationStructuresKHR(cmdList.vulkan_command_buffer(), m_buildInfos.size(), m_buildInfos.data(), m_buildRangePtrs.data());
}

}
