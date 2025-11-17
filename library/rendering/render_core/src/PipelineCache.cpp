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
   for (const auto i : Range(0u, state.descriptor_count)) {
      const auto& desc = state.descriptors[i];
      builder.add_descriptor_binding(desc.descriptor_type, desc.pipeline_stages, desc.descriptor_count);
   }
}

}// namespace

PipelineCache::PipelineCache(graphics_api::Device& device, resource::ResourceManager& resource_manager) :
    m_device(device),
    m_resource_manager(resource_manager)
{
}

graphics_api::Pipeline& PipelineCache::get_graphics_pipeline(const GraphicPipelineState& state)
{
   const auto hash = state.hash();

   if (const auto it = m_pipelines.find(hash); it != m_pipelines.end()) {
      return it->second;
   }

   log_debug("creating new graphics PSO (hash: {}).", hash);

   auto [pipeline_it, ok] = m_pipelines.emplace(hash, this->create_graphics_pso(state));
   assert(ok);

   return pipeline_it->second;
}

graphics_api::Pipeline& PipelineCache::get_compute_pipeline(const ComputePipelineState& state)
{
   const auto hash = state.hash();

   if (const auto it = m_pipelines.find(hash); it != m_pipelines.end()) {
      return it->second;
   }

   log_debug("creating new compute PSO (hash: {}).", hash);

   auto [pipeline_it, ok] = m_pipelines.emplace(hash, this->create_compute_pso(state));
   assert(ok);

   return pipeline_it->second;
}

graphics_api::ray_tracing::RayTracingPipeline& PipelineCache::get_ray_tracing_pso(const RayTracingPipelineState& state)
{
   const auto hash = state.hash();

   if (const auto it = m_ray_tracing_pipelines.find(hash); it != m_ray_tracing_pipelines.end()) {
      return it->second;
   }

   log_debug("creating new ray tracing PSO (hash: {}).", hash);

   auto [pipeline_it, ok] = m_ray_tracing_pipelines.emplace(hash, this->create_ray_tracing_pso(state));
   assert(ok);

   return pipeline_it->second;
}

graphics_api::ray_tracing::ShaderBindingTable& PipelineCache::get_shader_binding_table(const RayTracingPipelineState& state)
{
   const auto hash = state.hash();

   if (const auto it = m_ray_tracing_shader_binding_tables.find(hash); it != m_ray_tracing_shader_binding_tables.end()) {
      return it->second;
   }

   log_debug("creating shader binding table (hash: {}).", hash);

   auto [pipeline_it, ok] = m_ray_tracing_shader_binding_tables.emplace(hash, this->create_ray_tracing_shader_binding_table(state));
   assert(ok);

   return pipeline_it->second;
}

graphics_api::Pipeline PipelineCache::create_compute_pso(const ComputePipelineState& state) const
{
   gapi::ComputePipelineBuilder builder(m_device);
   builder.compute_shader(m_resource_manager.get(state.compute_shader.value()));

   write_descriptor_state(builder, state.descriptor_state);

   return GAPI_CHECK(builder.build());
}

graphics_api::Pipeline PipelineCache::create_graphics_pso(const GraphicPipelineState& state) const
{
   gapi::GraphicsPipelineBuilder builder(m_device);

   builder.vertex_shader(m_resource_manager.get(state.vertex_shader.value()));
   builder.fragment_shader(m_resource_manager.get(state.fragment_shader.value()));

   write_descriptor_state(builder, state.descriptor_state);

   if (state.vertex_layout.stride != 0) {
      builder.begin_vertex_layout_raw(state.vertex_layout.stride);
      for (const auto& attribute : state.vertex_layout.attributes) {
         builder.vertex_attribute(attribute.format, attribute.offset);
      }
      builder.end_vertex_layout();
   }

   for (const auto format : state.render_target_formats) {
      builder.color_attachment(format);
   }
   if (state.depth_target_format.has_value()) {
      builder.depth_attachment(*state.depth_target_format);
      builder.enable_depth_test(true);
   }
   builder.vertex_topology(state.vertex_topology);
   builder.depth_test_mode(state.depth_test_mode);
   builder.line_width(state.line_width);

   for (const auto push_constant : state.push_constants) {
      builder.push_constant(push_constant.flags, push_constant.size);
   }
   builder.enable_blending(state.is_blending_enabled);

   return GAPI_CHECK(builder.build());
}

graphics_api::ray_tracing::RayTracingPipeline PipelineCache::create_ray_tracing_pso(const RayTracingPipelineState& state) const
{
   gapi::ray_tracing::RayTracingPipelineBuilder builder(m_device);

   builder.ray_generation_shader(make_rt_shader_name(*state.ray_gen_shader), m_resource_manager.get(*state.ray_gen_shader));

   for (const auto shader : state.ray_miss_shaders) {
      builder.miss_shader(make_rt_shader_name(shader), m_resource_manager.get(shader));
   }
   for (const auto shader : state.ray_closest_hit_shaders) {
      builder.closest_hit_shader(make_rt_shader_name(shader), m_resource_manager.get(shader));
   }

   write_descriptor_state(builder, state.descriptor_state);

   for (const auto push_constant : state.push_constants) {
      builder.push_constant(push_constant.flags, push_constant.size);
   }

   builder.max_recursion(state.max_recursion);

   for (const auto& shader_group : state.shader_groups) {
      switch (shader_group.type) {
      case RayTracingShaderGroupType::General:
         builder.general_group(make_rt_shader_name(*shader_group.general_shader));
         break;
      case RayTracingShaderGroupType::Triangles: {
         std::array<Name, 2> shader_names{};

         u32 count = 0;
         if (shader_group.general_shader.has_value()) {
            shader_names[count++] = make_rt_shader_name(*shader_group.general_shader);
         }
         if (shader_group.closest_hit_shader.has_value()) {
            shader_names[count++] = make_rt_shader_name(*shader_group.closest_hit_shader);
         }

         if (count == 1) {
            builder.triangle_group(shader_names[0]);
         } else {
            builder.triangle_group(shader_names[1]);
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
