#pragma once

#include "../Buffer.hpp"

#include "triglav/Name.hpp"
#include "triglav/io/DynamicWriter.hpp"

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
   std::vector<u8> m_handle_buffer;
   MemorySize m_handle_size{};
   MemorySize m_handle_alignment{};
   MemorySize m_group_alignment{};
   PipelineStage m_last_stage{PipelineStage::None};

   MemorySize m_gen_rays_offset{};
   MemorySize m_miss_offset{};
   MemorySize m_hit_offset{};
   MemorySize m_callable_offset{};

   Index m_gen_rays_count{};
   Index m_miss_count{};
   Index m_hit_count{};
   Index m_callable_count{};
};

class ShaderBindingTable
{
 public:
   ShaderBindingTable(Buffer buffer, const VkStridedDeviceAddressRegionKHR& gen_rays_region,
                      const VkStridedDeviceAddressRegionKHR& miss_region, const VkStridedDeviceAddressRegionKHR& hit_region,
                      const VkStridedDeviceAddressRegionKHR& callable_region);

   [[nodiscard]] const VkStridedDeviceAddressRegionKHR& gen_rays_region() const;
   [[nodiscard]] const VkStridedDeviceAddressRegionKHR& miss_region() const;
   [[nodiscard]] const VkStridedDeviceAddressRegionKHR& hit_region() const;
   [[nodiscard]] const VkStridedDeviceAddressRegionKHR& callable_region() const;

 private:
   Buffer m_buffer;
   VkStridedDeviceAddressRegionKHR m_gen_rays_region;
   VkStridedDeviceAddressRegionKHR m_miss_region;
   VkStridedDeviceAddressRegionKHR m_hit_region;
   VkStridedDeviceAddressRegionKHR m_callable_region;
};

}// namespace triglav::graphics_api::ray_tracing