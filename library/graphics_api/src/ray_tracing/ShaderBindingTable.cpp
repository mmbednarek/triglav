#include "ray_tracing/ShaderBindingTable.hpp"
#include "Device.hpp"
#include "ray_tracing/RayTracingPipeline.hpp"
#include "vulkan/DynamicProcedures.hpp"

namespace triglav::graphics_api::ray_tracing {

namespace {

template <std::integral TIntegral>
constexpr TIntegral align_up(TIntegral x, size_t a) noexcept
{
   return TIntegral((x + (TIntegral(a) - 1)) & ~TIntegral(a - 1));
}

}

ShaderBindingTableBuilder::ShaderBindingTableBuilder(Device& device, RayTracingPipeline& pipeline) :
    m_device(device),
    m_pipeline(pipeline)
{
   VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProps{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
   VkPhysicalDeviceProperties2 props{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
   props.pNext = &rayTracingProps;

   vkGetPhysicalDeviceProperties2(device.vulkan_physical_device(), &props);

   m_handleSize = rayTracingProps.shaderGroupHandleSize;

   const auto bufferSize = m_handleSize * pipeline.shader_count();
   m_handleBuffer.resize(bufferSize);
   auto getHandlesResult = vulkan::vkGetRayTracingShaderGroupHandlesKHR(device.vulkan_device(), pipeline.vulkan_pipeline(), 0, pipeline.group_count(),
                                                m_handleBuffer.size(), m_handleBuffer.data());
   assert(getHandlesResult == VK_SUCCESS);
}

ShaderBindingTableBuilder& ShaderBindingTableBuilder::add_binding(Name name)
{
   auto [index, stage] = m_pipeline.shader_index(name);
   if (stage != m_lastStage) {
      switch (stage) {
      case PipelineStage::RayGenerationShader:
         m_genRaysOffset = m_writer.size();
         break;
      case PipelineStage::MissShader:
         m_missOffset = m_writer.size();
         break;
      case PipelineStage::AnyHitShader:
         [[fallthrough]];
      case PipelineStage::ClosestHitShader:
         m_hitOffset = m_writer.size();
         break;
      case PipelineStage::CallableShader:
         m_callableOffset = m_writer.size();
         break;
      default:
         break;
      }
      m_lastStage = stage;
   }

   switch (stage) {
   case PipelineStage::RayGenerationShader:
      ++m_genRaysCount;
      break;
   case PipelineStage::MissShader:
      ++m_missCount;
      break;
   case PipelineStage::AnyHitShader:
      [[fallthrough]];
   case PipelineStage::ClosestHitShader:
      ++m_hitCount;
      break;
   case PipelineStage::CallableShader:
      ++m_callableCount;
      break;
   default:
      break;
   }

   assert(m_writer.write({this->handle(index), m_handleSize}).has_value());
   return *this;
}

u8* ShaderBindingTableBuilder::handle(Index index)
{
   return m_handleBuffer.data() + m_handleSize * index;
}

ShaderBindingTable ShaderBindingTableBuilder::build()
{
   auto buffer = GAPI_CHECK(m_device.create_buffer(BufferUsage::TransferDst | BufferUsage::ShaderBindingTable, m_writer.size()));
   GAPI_CHECK_STATUS(buffer.write_indirect(m_writer.data(), m_writer.size()));

   const auto bufferAddr = buffer.vulkan_device_address();

   VkStridedDeviceAddressRegionKHR rayGenAddress{};
   rayGenAddress.deviceAddress = bufferAddr + m_genRaysOffset;
   rayGenAddress.stride = m_genRaysCount == 0 ? 0 : m_handleSize;
   rayGenAddress.size = m_handleSize * m_genRaysCount;

   VkStridedDeviceAddressRegionKHR missAddress{};
   missAddress.deviceAddress = bufferAddr + m_missOffset;
   missAddress.stride = m_missCount == 0 ? 0 : m_handleSize;
   missAddress.size = m_handleSize * m_missCount;

   VkStridedDeviceAddressRegionKHR hitAddress{};
   hitAddress.deviceAddress = bufferAddr + m_hitOffset;
   hitAddress.stride = m_hitCount == 0 ? 0 : m_handleSize;
   hitAddress.size = m_handleSize * m_hitCount;

   VkStridedDeviceAddressRegionKHR callableAddress{};
   callableAddress.deviceAddress = bufferAddr + m_callableOffset;
   callableAddress.stride = m_callableCount == 0 ? 0 : m_handleSize;
   callableAddress.size = m_handleSize * m_callableCount;

   return ShaderBindingTable{std::move(buffer), rayGenAddress, missAddress, hitAddress, callableAddress};
}


ShaderBindingTable::ShaderBindingTable(Buffer buffer, const VkStridedDeviceAddressRegionKHR& genRaysRegion,
                                       const VkStridedDeviceAddressRegionKHR& missRegion, const VkStridedDeviceAddressRegionKHR& hitRegion,
                                       const VkStridedDeviceAddressRegionKHR& callableRegion) :
    m_buffer(std::move(buffer)),
    m_genRaysRegion(genRaysRegion),
    m_missRegion(missRegion),
    m_hitRegion(hitRegion),
    m_callableRegion(callableRegion)
{
}

const VkStridedDeviceAddressRegionKHR& ShaderBindingTable::gen_rays_region() const
{
   return m_genRaysRegion;
}

const VkStridedDeviceAddressRegionKHR& ShaderBindingTable::miss_region() const
{
   return m_missRegion;
}

const VkStridedDeviceAddressRegionKHR& ShaderBindingTable::hit_region() const
{
   return m_hitRegion;
}

const VkStridedDeviceAddressRegionKHR& ShaderBindingTable::callable_region() const
{
   return m_callableRegion;
}

}// namespace triglav::graphics_api::ray_tracing
