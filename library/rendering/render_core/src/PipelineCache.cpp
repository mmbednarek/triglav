#include "PipelineCache.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/PipelineBuilder.hpp"
#include "triglav/graphics_api/ray_tracing/RayTracingPipeline.hpp"
#include "triglav/resource/ResourceManager.hpp"

namespace triglav::render_core {

namespace gapi = graphics_api;
namespace rt = graphics_api::ray_tracing;

namespace {

void write_descriptor_state(graphics_api::PipelineBuilderBase& builder, const DescriptorState& state)
{
   for (const auto i : Range(0u, state.descriptorCount)) {
      const auto& desc = state.descriptors[i];
      builder.add_descriptor_binding(desc.descriptorType, desc.pipelineStages, desc.descriptorCount);
   }
}

}// namespace

PipelineCache::PipelineCache(graphics_api::Device& device, resource::ResourceManager& resourceManager) :
    m_device(device),
    m_resourceManager(resourceManager)
{
}

graphics_api::Pipeline& PipelineCache::get_graphics_pipeline(const GraphicPipelineState& state)
{
   const auto hash = state.hash();

   if (const auto it = m_pipelines.find(hash); it != m_pipelines.end()) {
      return it->second;
   }

   log_debug("creating new graphics PSO (hash: {}).", hash);

   auto [pipelineIt, ok] = m_pipelines.emplace(hash, this->create_graphics_pso(state));
   assert(ok);

   return pipelineIt->second;
}

graphics_api::Pipeline& PipelineCache::get_compute_pipeline(const ComputePipelineState& state)
{
   const auto hash = state.hash();

   if (const auto it = m_pipelines.find(hash); it != m_pipelines.end()) {
      return it->second;
   }

   log_debug("creating new compute PSO (hash: {}).", hash);

   auto [pipelineIt, ok] = m_pipelines.emplace(hash, this->create_compute_pso(state));
   assert(ok);

   return pipelineIt->second;
}

graphics_api::ray_tracing::RayTracingPipeline& PipelineCache::get_ray_tracing_pso(const RayTracingPipelineState& state)
{
   const auto hash = state.hash();

   if (const auto it = m_rayTracingPipelines.find(hash); it != m_rayTracingPipelines.end()) {
      return it->second;
   }

   log_debug("creating new ray tracing PSO (hash: {}).", hash);

   auto [pipelineIt, ok] = m_rayTracingPipelines.emplace(hash, this->create_ray_tracing_pso(state));
   assert(ok);

   return pipelineIt->second;
}

graphics_api::ray_tracing::ShaderBindingTable& PipelineCache::get_shader_binding_table(const RayTracingPipelineState& state)
{
   const auto hash = state.hash();

   if (const auto it = m_rayTracingShaderBindingTables.find(hash); it != m_rayTracingShaderBindingTables.end()) {
      return it->second;
   }

   log_debug("creating shader binding table (hash: {}).", hash);

   auto [pipelineIt, ok] = m_rayTracingShaderBindingTables.emplace(hash, this->create_ray_tracing_shader_binding_table(state));
   assert(ok);

   return pipelineIt->second;
}

graphics_api::Pipeline PipelineCache::create_compute_pso(const ComputePipelineState& state) const
{
   gapi::ComputePipelineBuilder builder(m_device);
   builder.compute_shader(m_resourceManager.get(state.computeShader.value()));

   write_descriptor_state(builder, state.descriptorState);

   return GAPI_CHECK(builder.build());
}

graphics_api::Pipeline PipelineCache::create_graphics_pso(const GraphicPipelineState& state) const
{
   gapi::GraphicsPipelineBuilder builder(m_device);

   builder.vertex_shader(m_resourceManager.get(state.vertexShader.value()));
   builder.fragment_shader(m_resourceManager.get(state.fragmentShader.value()));

   write_descriptor_state(builder, state.descriptorState);

   if (state.vertexLayout.stride != 0) {
      builder.begin_vertex_layout_raw(state.vertexLayout.stride);
      for (const auto& attribute : state.vertexLayout.attributes) {
         builder.vertex_attribute(attribute.format, attribute.offset);
      }
      builder.end_vertex_layout();
   }

   for (const auto format : state.renderTargetFormats) {
      builder.color_attachment(format);
   }
   if (state.depthTargetFormat.has_value()) {
      builder.depth_attachment(*state.depthTargetFormat);
      builder.enable_depth_test(true);
   }
   builder.vertex_topology(state.vertexTopology);
   builder.depth_test_mode(state.depthTestMode);
   builder.line_width(state.lineWidth);

   for (const auto pushConstant : state.pushConstants) {
      builder.push_constant(pushConstant.flags, pushConstant.size);
   }
   builder.enable_blending(state.isBlendingEnabled);

   return GAPI_CHECK(builder.build());
}

graphics_api::ray_tracing::RayTracingPipeline PipelineCache::create_ray_tracing_pso(const RayTracingPipelineState& state) const
{
   gapi::ray_tracing::RayTracingPipelineBuilder builder(m_device);

   builder.ray_generation_shader(make_rt_shader_name(*state.rayGenShader), m_resourceManager.get(*state.rayGenShader));

   for (const auto shader : state.rayMissShaders) {
      builder.miss_shader(make_rt_shader_name(shader), m_resourceManager.get(shader));
   }
   for (const auto shader : state.rayClosestHitShaders) {
      builder.closest_hit_shader(make_rt_shader_name(shader), m_resourceManager.get(shader));
   }

   write_descriptor_state(builder, state.descriptorState);

   for (const auto pushConstant : state.pushConstants) {
      builder.push_constant(pushConstant.flags, pushConstant.size);
   }

   builder.max_recursion(state.maxRecursion);

   for (const auto& shaderGroup : state.shaderGroups) {
      switch (shaderGroup.type) {
      case RayTracingShaderGroupType::General:
         builder.general_group(make_rt_shader_name(*shaderGroup.generalShader));
         break;
      case RayTracingShaderGroupType::Triangles: {
         std::array<Name, 2> shaderNames{};

         u32 count = 0;
         if (shaderGroup.generalShader.has_value()) {
            shaderNames[count++] = make_rt_shader_name(*shaderGroup.generalShader);
         }
         if (shaderGroup.closestHitShader.has_value()) {
            shaderNames[count++] = make_rt_shader_name(*shaderGroup.closestHitShader);
         }

         if (count == 1) {
            builder.triangle_group(shaderNames[0]);
         } else {
            builder.triangle_group(shaderNames[1]);
         }
         break;
      }
      default:
         break;
      }
   }

   return GAPI_CHECK(builder.build());
}

rt::ShaderBindingTable PipelineCache::create_ray_tracing_shader_binding_table(const RayTracingPipelineState& state)
{
   auto& pso = this->get_ray_tracing_pso(state);
   rt::ShaderBindingTableBuilder builder(m_device, pso);

   const auto bindings = state.shader_bindings();
   for (const auto binding : bindings) {
      builder.add_binding(binding);
   }

   return builder.build();
}

}// namespace triglav::render_core
