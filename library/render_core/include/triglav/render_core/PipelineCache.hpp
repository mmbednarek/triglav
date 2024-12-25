#pragma once

#include "RenderCore.hpp"

#include <unordered_map>

namespace triglav::graphics_api {

class Pipeline;
class Device;
class PipelineBuilderBase;

}// namespace triglav::graphics_api

namespace triglav::resource {

class ResourceManager;

}

namespace triglav::render_core {

class PipelineCache
{
 public:
   PipelineCache(graphics_api::Device& device, resource::ResourceManager& resourceManager);

   graphics_api::Pipeline& get_graphics_pipeline(const GraphicPipelineState& state);
   graphics_api::Pipeline& get_compute_pipeline(const ComputePipelineState& state);

 private:
   graphics_api::Device& m_device;
   resource::ResourceManager& m_resourceManager;

   [[nodiscard]] graphics_api::Pipeline create_compute_pso(const ComputePipelineState& state) const;
   [[nodiscard]] graphics_api::Pipeline create_graphics_pso(const GraphicPipelineState& state) const;

   std::unordered_map<PipelineHash, graphics_api::Pipeline> m_pipelines;
};


}// namespace triglav::render_core