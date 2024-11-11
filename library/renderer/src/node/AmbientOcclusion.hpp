#pragma once

#include "triglav/graphics_api/Framebuffer.hpp"
#include "triglav/graphics_api/RenderTarget.hpp"
#include "triglav/render_core/IRenderNode.hpp"

#include "AmbientOcclusionRenderer.hpp"
#include "Scene.hpp"

namespace triglav::renderer::node {

class AmbientOcclusion : public render_core::IRenderNode
{
 public:
   AmbientOcclusion(graphics_api::Device& device, resource::ResourceManager& resourceManager, Scene& scene);
   std::unique_ptr<render_core::NodeFrameResources> create_node_resources() override;
   [[nodiscard]] graphics_api::WorkTypeFlags work_types() const override;
   void record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& resources,
                        graphics_api::CommandList& cmdList) override;

 private:
   graphics_api::RenderTarget m_renderTarget;
   AmbientOcclusionRenderer m_renderer;
   Scene& m_scene;
};

}// namespace triglav::renderer::node
