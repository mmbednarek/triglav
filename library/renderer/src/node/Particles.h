#pragma once

#include "triglav/render_core/IRenderNode.hpp"
#include "triglav/resource/ResourceManager.h"

#include <glm/mat4x4.hpp>

namespace triglav::render_core {
class RenderGraph;
}
namespace triglav::renderer::node {

class ParticlesResources : public render_core::NodeFrameResources
{
 public:
   explicit ParticlesResources(graphics_api::Device &device);
   [[nodiscard]] graphics_api::Buffer &particles_buffer();

 private:
   graphics_api::Device &m_device;
   graphics_api::Buffer m_particlesBuffer;
};

class Particles : public render_core::IRenderNode
{
 public:
   Particles(graphics_api::Device &device, resource::ResourceManager& resourceManager, render_core::RenderGraph &renderGraph);

   [[nodiscard]] graphics_api::WorkTypeFlags work_types() const override;
   std::unique_ptr<render_core::NodeFrameResources> create_node_resources() override;
   void record_commands(render_core::FrameResources &frameResources, render_core::NodeFrameResources &resources, graphics_api::CommandList &cmdList) override;

 private:
   graphics_api::Device &m_device;
   render_core::RenderGraph &m_renderGraph;
   graphics_api::Pipeline m_computePipeline;
};

}
