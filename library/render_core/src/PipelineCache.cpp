#include "PipelineCache.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/PipelineBuilder.hpp"
#include "triglav/resource/ResourceManager.hpp"

#include <spdlog/spdlog.h>

namespace triglav::render_core {

namespace gapi = graphics_api;

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

   spdlog::debug("pso-cache: creating new graphics PSO (hash: {}).", hash);

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

   spdlog::debug("pso-cache: creating new compute PSO (hash: {}).", hash);

   auto [pipelineIt, ok] = m_pipelines.emplace(hash, this->create_compute_pso(state));
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

   return GAPI_CHECK(builder.build());
}


}// namespace triglav::render_core
