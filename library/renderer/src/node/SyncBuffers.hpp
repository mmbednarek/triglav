#pragma once

#include "triglav/render_core/IRenderNode.hpp"
#include "triglav/render_core/RenderGraph.hpp"

#include "Scene.hpp"

namespace triglav::renderer::node {

class SyncBuffers : public render_core::IRenderNode
{
 public:
   explicit SyncBuffers(Scene& scene);

   [[nodiscard]] graphics_api::WorkTypeFlags work_types() const override;
   void record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& resources,
                        graphics_api::CommandList& cmdList) override;
   std::unique_ptr<render_core::NodeFrameResources> create_node_resources() override;

 private:
   Scene& m_scene;
};

}// namespace triglav::renderer::node