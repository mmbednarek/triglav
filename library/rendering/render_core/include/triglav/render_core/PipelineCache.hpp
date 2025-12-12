#pragma once

#include "RenderCore.hpp"

#include "triglav/Logging.hpp"
#include "triglav/graphics_api/PipelineBuilder.hpp"
#include "triglav/graphics_api/ray_tracing/RayTracingPipeline.hpp"
#include "triglav/graphics_api/ray_tracing/ShaderBindingTable.hpp"

#include <unordered_map>

namespace triglav::resource {

class ResourceManager;

}

namespace triglav::render_core {

class PipelineCache
{
   TG_DEFINE_LOG_CATEGORY(PipelineCache)
 public:
   PipelineCache(graphics_api::Device& device, resource::ResourceManager& resource_manager);

   [[nodiscard]] graphics_api::Pipeline& get_graphics_pipeline(const GraphicPipelineState& state);
   [[nodiscard]] graphics_api::Pipeline& get_compute_pipeline(const ComputePipelineState& state);
   [[nodiscard]] graphics_api::ray_tracing::RayTracingPipeline& get_ray_tracing_pso(const RayTracingPipelineState& state);
   [[nodiscard]] graphics_api::ray_tracing::ShaderBindingTable& get_shader_binding_table(const RayTracingPipelineState& state);

 private:
   graphics_api::Device& m_device;
   resource::ResourceManager& m_resource_manager;

   [[nodiscard]] graphics_api::Pipeline create_compute_pso(const ComputePipelineState& state) const;
   [[nodiscard]] graphics_api::Pipeline create_graphics_pso(const GraphicPipelineState& state) const;
   [[nodiscard]] graphics_api::ray_tracing::RayTracingPipeline create_ray_tracing_pso(const RayTracingPipelineState& state) const;

   [[nodiscard]] graphics_api::ray_tracing::ShaderBindingTable
   create_ray_tracing_shader_binding_table(const RayTracingPipelineState& state);

   std::unordered_map<PipelineHash, graphics_api::Pipeline> m_pipelines;
   std::unordered_map<PipelineHash, graphics_api::ray_tracing::RayTracingPipeline> m_ray_tracing_pipelines;
   std::unordered_map<PipelineHash, graphics_api::ray_tracing::ShaderBindingTable> m_ray_tracing_shader_binding_tables;
};

}// namespace triglav::render_core