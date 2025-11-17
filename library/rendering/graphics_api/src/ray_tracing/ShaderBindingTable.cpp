#include "ray_tracing/ShaderBindingTable.hpp"
#include "Device.hpp"
#include "ray_tracing/RayTracingPipeline.hpp"
#include "vulkan/DynamicProcedures.hpp"

namespace triglav::graphics_api::ray_tracing {

namespace {

template<std::integral TIntegral>
constexpr TIntegral align_up(TIntegral x, size_t a) noexcept
{
   return TIntegral((x + (TIntegral(a) - 1)) & ~TIntegral(a - 1));
}

}// namespace

ShaderBindingTableBuilder::ShaderBindingTableBuilder(Device& device, RayTracingPipeline& pipeline) :
    m_device(device),
    m_pipeline(pipeline)
{
   VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_props{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
   VkPhysicalDeviceProperties2 props{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
   props.pNext = &ray_tracing_props;

   vkGetPhysicalDeviceProperties2(device.vulkan_physical_device(), &props);

   m_handle_size = ray_tracing_props.shaderGroupHandleSize;
   m_handle_alignment = ray_tracing_props.shaderGroupHandleAlignment;
   m_group_alignment = ray_tracing_props.shaderGroupBaseAlignment;

   const auto buffer_size = m_handle_size * pipeline.shader_count();
   m_handle_buffer.resize(buffer_size);
   [[maybe_unused]] const auto get_handles_result = vulkan::vkGetRayTracingShaderGroupHandlesKHR(
      device.vulkan_device(), pipeline.vulkan_pipeline(), 0, pipeline.group_count(), m_handle_buffer.size(), m_handle_buffer.data());
   assert(get_handles_result == VK_SUCCESS);
}

ShaderBindingTableBuilder& ShaderBindingTableBuilder::add_binding(const Name name)
{
   assert(m_writer.align(m_handle_alignment).has_value());

   auto [index, stage] = m_pipeline.shader_index(name);
   if (stage != m_last_stage) {
      assert(m_writer.align(m_group_alignment).has_value());

      switch (stage) {
      case PipelineStage::RayGenerationShader:
         m_gen_rays_offset = m_writer.size();
         break;
      case PipelineStage::MissShader:
         m_miss_offset = m_writer.size();
         break;
      case PipelineStage::AnyHitShader:
         [[fallthrough]];
      case PipelineStage::ClosestHitShader:
         m_hit_offset = m_writer.size();
         break;
      case PipelineStage::CallableShader:
         m_callable_offset = m_writer.size();
         break;
      default:
         break;
      }
      m_last_stage = stage;
   }

   switch (stage) {
   case PipelineStage::RayGenerationShader:
      ++m_gen_rays_count;
      break;
   case PipelineStage::MissShader:
      ++m_miss_count;
      break;
   case PipelineStage::AnyHitShader:
      [[fallthrough]];
   case PipelineStage::ClosestHitShader:
      ++m_hit_count;
      break;
   case PipelineStage::CallableShader:
      ++m_callable_count;
      break;
   default:
      break;
   }

   assert(m_writer.write({this->handle(index), m_handle_size}).has_value());
   return *this;
}

u8* ShaderBindingTableBuilder::handle(const Index index)
{
   return m_handle_buffer.data() + m_handle_size * index;
}

ShaderBindingTable ShaderBindingTableBuilder::build()
{
   auto buffer = GAPI_CHECK(m_device.create_buffer(BufferUsage::TransferDst | BufferUsage::ShaderBindingTable, m_writer.size()));
   GAPI_CHECK_STATUS(buffer.write_indirect(m_writer.data(), m_writer.size()));

   const auto buffer_addr = buffer.vulkan_device_address();

   VkStridedDeviceAddressRegionKHR ray_gen_address{};
   ray_gen_address.deviceAddress = buffer_addr + m_gen_rays_offset;
   ray_gen_address.stride = m_gen_rays_count == 0 ? 0 : m_handle_size;
   ray_gen_address.size = m_handle_size * m_gen_rays_count;

   VkStridedDeviceAddressRegionKHR miss_address{};
   miss_address.deviceAddress = buffer_addr + m_miss_offset;
   miss_address.stride = m_miss_count == 0 ? 0 : m_handle_size;
   miss_address.size = m_handle_size * m_miss_count;

   VkStridedDeviceAddressRegionKHR hit_address{};
   hit_address.deviceAddress = buffer_addr + m_hit_offset;
   hit_address.stride = m_hit_count == 0 ? 0 : m_handle_size;
   hit_address.size = m_handle_size * m_hit_count;

   VkStridedDeviceAddressRegionKHR callable_address{};
   callable_address.deviceAddress = buffer_addr + m_callable_offset;
   callable_address.stride = m_callable_count == 0 ? 0 : m_handle_size;
   callable_address.size = m_handle_size * m_callable_count;

   return ShaderBindingTable{std::move(buffer), ray_gen_address, miss_address, hit_address, callable_address};
}


ShaderBindingTable::ShaderBindingTable(Buffer buffer, const VkStridedDeviceAddressRegionKHR& gen_rays_region,
                                       const VkStridedDeviceAddressRegionKHR& miss_region,
                                       const VkStridedDeviceAddressRegionKHR& hit_region,
                                       const VkStridedDeviceAddressRegionKHR& callable_region) :
    m_buffer(std::move(buffer)),
    m_gen_rays_region(gen_rays_region),
    m_miss_region(miss_region),
    m_hit_region(hit_region),
    m_callable_region(callable_region)
{
}

const VkStridedDeviceAddressRegionKHR& ShaderBindingTable::gen_rays_region() const
{
   return m_gen_rays_region;
}

const VkStridedDeviceAddressRegionKHR& ShaderBindingTable::miss_region() const
{
   return m_miss_region;
}

const VkStridedDeviceAddressRegionKHR& ShaderBindingTable::hit_region() const
{
   return m_hit_region;
}

const VkStridedDeviceAddressRegionKHR& ShaderBindingTable::callable_region() const
{
   return m_callable_region;
}

}// namespace triglav::graphics_api::ray_tracing
