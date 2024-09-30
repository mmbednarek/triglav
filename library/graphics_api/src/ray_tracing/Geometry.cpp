#include "ray_tracing/Geometry.hpp"
#include "../vulkan/Util.hpp"
#include "CommandList.hpp"
#include "Device.hpp"

namespace triglav::graphics_api::ray_tracing {

void GeometryBuildInfo::add_triangles(const Buffer& vertexBuffer, const Buffer& indexBuffer, ColorFormat vertexFormat,
                                      MemorySize vertexSize, u32 vertexCount, u32 triangleCount)
{
   VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
   geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
   geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
   geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
   geometry.geometry.triangles.pNext = nullptr;
   geometry.geometry.triangles.vertexData.deviceAddress = vertexBuffer.vulkan_device_address();
   geometry.geometry.triangles.indexData.deviceAddress = indexBuffer.vulkan_device_address();
   geometry.geometry.triangles.vertexStride = vertexSize;
   geometry.geometry.triangles.vertexFormat = graphics_api::vulkan::to_vulkan_color_format(vertexFormat).value_or(VK_FORMAT_MAX_ENUM);
   geometry.geometry.triangles.maxVertex = vertexCount - 1;
   geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
   geometry.geometry.triangles.transformData.deviceAddress = 0;
   m_geometries.push_back(geometry);

   VkAccelerationStructureBuildRangeInfoKHR ranges{};
   ranges.primitiveCount = triangleCount;
   m_ranges.push_back(ranges);

   m_primitiveCounts.push_back(triangleCount);
}

void GeometryBuildInfo::add_bounding_boxes(const Buffer& boundingBoxBuffer, MemorySize bbSize, u32 bbCount)
{
   VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
   geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
   geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
   geometry.geometry.aabbs.pNext = nullptr;
   geometry.geometry.aabbs.data.deviceAddress = boundingBoxBuffer.vulkan_device_address();
   geometry.geometry.aabbs.stride = bbSize;
   m_geometries.push_back(geometry);

   VkAccelerationStructureBuildRangeInfoKHR ranges{};
   ranges.primitiveCount = bbCount;
   m_ranges.push_back(ranges);

   m_primitiveCounts.push_back(bbCount);
}

void GeometryBuildInfo::add_instances(const Buffer& instanceBuffer, u32 instanceCount)
{
   VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
   geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
   geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
   geometry.geometry.instances.pNext = nullptr;
   geometry.geometry.instances.arrayOfPointers = false;
   geometry.geometry.instances.data.deviceAddress = instanceBuffer.vulkan_device_address();
   m_geometries.push_back(geometry);

   VkAccelerationStructureBuildRangeInfoKHR ranges{};
   ranges.primitiveCount = instanceCount;
   m_ranges.push_back(ranges);

   m_primitiveCounts.push_back(instanceCount);
}

VkAccelerationStructureBuildSizesInfoKHR GeometryBuildInfo::size_requirement(Device& device) const
{
   VkAccelerationStructureBuildSizesInfoKHR outInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
   vulkan::vkGetAccelerationStructureBuildSizesKHR(device.vulkan_device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &m_buildInfo,
                                                   m_primitiveCounts.data(), &outInfo);

   return outInfo;
}

VkAccelerationStructureBuildGeometryInfoKHR GeometryBuildInfo::build(AccelerationStructure& dstAs, const BufferHeap::Section& scratchBuffer)
{
   m_buildInfo.dstAccelerationStructure = dstAs.vulkan_acceleration_structure();
   m_buildInfo.scratchData.deviceAddress = scratchBuffer.buffer->vulkan_device_address() + scratchBuffer.offset;
   m_lastAccelerationStructure = &dstAs;
   return m_buildInfo;
}

VkAccelerationStructureBuildRangeInfoKHR* GeometryBuildInfo::ranges()
{
   return m_ranges.data();
}
void GeometryBuildInfo::finalize(VkAccelerationStructureTypeKHR accelerationStructureType)
{
   m_buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
   m_buildInfo.pNext = nullptr;
   m_buildInfo.type = accelerationStructureType;
   m_buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
   m_buildInfo.srcAccelerationStructure =
      m_lastAccelerationStructure == nullptr ? nullptr : m_lastAccelerationStructure->vulkan_acceleration_structure();
   m_buildInfo.mode = m_lastAccelerationStructure == nullptr ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR
                                                             : VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
   m_buildInfo.dstAccelerationStructure = nullptr;
   m_buildInfo.geometryCount = m_geometries.size();
   m_buildInfo.pGeometries = m_geometries.data();
   m_buildInfo.scratchData.deviceAddress = 0;
}

// *********************************************
// ****** GEOMETRY BUILD CONTEXT ***************
// *********************************************

GeometryBuildContext::GeometryBuildContext(Device& device, AccelerationStructurePool& asPool, BufferHeap& scratchBufferHeap) :
    m_device(device),
    m_asPool(asPool),
    m_scratchBufferHeap(scratchBufferHeap)
{
}

void GeometryBuildContext::add_triangle_buffer(const Buffer& vertexBuffer, const Buffer& indexBuffer, ColorFormat vertexFormat,
                                               MemorySize vertexSize, u32 maxVertexCount, u32 triangleCount)
{
   m_currentTriangles.add_triangles(vertexBuffer, indexBuffer, vertexFormat, vertexSize, maxVertexCount, triangleCount);
}

void GeometryBuildContext::add_bounding_box_buffer(const Buffer& boundingBoxBuffer, MemorySize bbSize, u32 bbCount)
{
   m_currentBoundingBoxes.add_bounding_boxes(boundingBoxBuffer, bbSize, bbCount);
}

void GeometryBuildContext::add_instance_buffer(const Buffer& instanceBuffer, u32 instanceCount)
{
   m_currentInstances.add_instances(instanceBuffer, instanceCount);
}

AccelerationStructure* GeometryBuildContext::commit_triangles()
{
   auto& geo = m_triangleGeometries.emplace_back(std::move(m_currentTriangles));

   geo.finalize(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);
   const auto requirement = geo.size_requirement(m_device);

   auto* nextAccelerationStruct =
      m_asPool.acquire_acceleration_structure(AccelerationStructureType::BottomLevel, requirement.accelerationStructureSize);
   auto scratchSection = m_scratchBufferHeap.allocate_section(requirement.buildScratchSize);

   m_buildInfos.push_back(geo.build(*nextAccelerationStruct, scratchSection));
   m_buildRangePtrs.push_back(geo.ranges());

   return nextAccelerationStruct;
}

AccelerationStructure* GeometryBuildContext::commit_bounding_boxes()
{
   auto& geo = m_boundingBoxesGeometries.emplace_back(std::move(m_currentBoundingBoxes));

   geo.finalize(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);
   const auto requirement = geo.size_requirement(m_device);

   auto* nextAccelerationStruct =
      m_asPool.acquire_acceleration_structure(AccelerationStructureType::BottomLevel, requirement.accelerationStructureSize);
   auto scratchSection = m_scratchBufferHeap.allocate_section(requirement.buildScratchSize);

   m_buildInfos.push_back(geo.build(*nextAccelerationStruct, scratchSection));
   m_buildRangePtrs.push_back(geo.ranges());

   return nextAccelerationStruct;
}

AccelerationStructure* GeometryBuildContext::commit_instances()
{
   auto& geo = m_instanceGeometries.emplace_back(std::move(m_currentInstances));

   geo.finalize(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);
   const auto requirement = geo.size_requirement(m_device);

   auto* nextAccelerationStruct =
      m_asPool.acquire_acceleration_structure(AccelerationStructureType::TopLevel, requirement.accelerationStructureSize);
   auto scratchSection = m_scratchBufferHeap.allocate_section(requirement.buildScratchSize);

   m_buildInfos.push_back(geo.build(*nextAccelerationStruct, scratchSection));
   m_buildRangePtrs.push_back(geo.ranges());

   return nextAccelerationStruct;
}

void GeometryBuildContext::build_acceleration_structures(CommandList& cmdList)
{
   vulkan::vkCmdBuildAccelerationStructuresKHR(cmdList.vulkan_command_buffer(), m_buildInfos.size(), m_buildInfos.data(),
                                               m_buildRangePtrs.data());
}

}// namespace triglav::graphics_api::ray_tracing
