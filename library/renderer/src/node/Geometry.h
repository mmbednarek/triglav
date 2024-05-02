#pragma once

#include "triglav/render_core/IRenderNode.hpp"

#include "Scene.h"
#include "SkyBox.h"

#include <GroundRenderer.h>

namespace triglav::renderer::node {

class Geometry : public render_core::IRenderNode
{
 public:
   Geometry(graphics_api::Device &device, resource::ResourceManager &resourceManager, Scene& scene);

   [[nodiscard]] graphics_api::WorkTypeFlags work_types() const override;
   void record_commands(render_core::FrameResources &frameResources,
                        render_core::NodeFrameResources &resources,
                        graphics_api::CommandList &cmdList) override;
   std::unique_ptr<render_core::NodeFrameResources> create_node_resources() override;

   [[nodiscard]] float gpu_time() const;

 private:
   graphics_api::Device &m_device;
   resource::ResourceManager &m_resourceManager;
   Scene& m_scene;
   graphics_api::RenderTarget m_renderTarget;
   graphics_api::Pipeline m_pipeline;
   SkyBox m_skybox;
   GroundRenderer m_groundRenderer;
   DebugLinesRenderer m_debugLinesRenderer;
   graphics_api::TimestampArray m_timestampArray;
};

}// namespace triglav::renderer::node
