#pragma once

#include "../Buffer.hpp"

#include "triglav/io/DynamicWriter.hpp"
#include "triglav/Name.hpp"

namespace triglav::graphics_api {
class Device;
}

namespace triglav::graphics_api::ray_tracing {

class RayTracingPipeline;
class ShaderBindingTable;

class ShaderBindingTableBuilder
{
 public:
   ShaderBindingTableBuilder(Device& device, RayTracingPipeline& pipeline);

   ShaderBindingTableBuilder& add_binding(Name name);
   ShaderBindingTable build();

 private:
   u8* handle(Index index);

   io::DynamicWriter m_writer;
   Device& m_device;
   RayTracingPipeline& m_pipeline;
   std::vector<u8> m_handleBuffer;
   MemorySize m_handleSize{};
   PipelineStage m_lastStage{PipelineStage::None};

   MemorySize m_genRaysOffset{};
   MemorySize m_missOffset{};
   MemorySize m_hitOffset{};
   MemorySize m_callableOffset{};

   Index m_genRaysCount{};
   Index m_missCount{};
   Index m_hitCount{};
   Index m_callableCount{};
};

class ShaderBindingTable
{
 public:
   ShaderBindingTable(Buffer buffer, const VkStridedDeviceAddressRegionKHR& genRaysRegion,
                      const VkStridedDeviceAddressRegionKHR& missRegion, const VkStridedDeviceAddressRegionKHR& hitRegion,
                      const VkStridedDeviceAddressRegionKHR& callableRegion);

   [[nodiscard]] const VkStridedDeviceAddressRegionKHR& gen_rays_region() const;
   [[nodiscard]] const VkStridedDeviceAddressRegionKHR& miss_region() const;
   [[nodiscard]] const VkStridedDeviceAddressRegionKHR& hit_region() const;
   [[nodiscard]] const VkStridedDeviceAddressRegionKHR& callable_region() const;

 private:
   Buffer m_buffer;
   VkStridedDeviceAddressRegionKHR m_genRaysRegion;
   VkStridedDeviceAddressRegionKHR m_missRegion;
   VkStridedDeviceAddressRegionKHR m_hitRegion;
   VkStridedDeviceAddressRegionKHR m_callableRegion;
};

}// namespace triglav::graphics_api::ray_tracing