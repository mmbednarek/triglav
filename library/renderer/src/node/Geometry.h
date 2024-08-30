#pragma once

#include "triglav/render_core/IRenderNode.hpp"

#include "MaterialManager.h"
#include "Scene.h"
#include "SkyBox.h"

#include <GroundRenderer.h>

namespace triglav::render_core {
class RenderGraph;
}
namespace triglav::renderer::node {


class IGeometryResources : public render_core::NodeFrameResources
{
 public:
   [[nodiscard]] virtual GroundRenderer::UniformBuffer& ground_ubo() = 0;
};

class Geometry : public render_core::IRenderNode
{
 public:
   Geometry(graphics_api::Device& device, resource::ResourceManager& resourceManager, Scene& scene, render_core::RenderGraph& renderGraph);

   [[nodiscard]] graphics_api::WorkTypeFlags work_types() const override;
   void record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& resources,
                        graphics_api::CommandList& cmdList) override;
   std::unique_ptr<render_core::NodeFrameResources> create_node_resources() override;

   [[nodiscard]] float gpu_time() const;
   [[nodiscard]] const GroundRenderer& ground_renderer() const;

 private:
   graphics_api::Device& m_device;
   resource::ResourceManager& m_resourceManager;
   Scene& m_scene;
   render_core::RenderGraph& m_renderGraph;
   graphics_api::RenderTarget m_renderTarget;
   MaterialManager m_materialManager;
   SkyBox m_skybox;
   GroundRenderer m_groundRenderer;
   DebugLinesRenderer m_debugLinesRenderer;
   graphics_api::TimestampArray m_timestampArray;
};

}// namespace triglav::renderer::node
