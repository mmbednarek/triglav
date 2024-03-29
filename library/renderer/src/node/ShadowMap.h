#pragma once

#include "triglav/render_core/IRenderNode.hpp"

#include "Scene.h"
#include "ShadowMapRenderer.h"

namespace triglav::renderer::node {

class ShadowMap : public render_core::IRenderNode
{
public:
   ShadowMap(Scene &scene, ShadowMapRenderer &renderer);

  [[nodiscard]] graphics_api::WorkTypeFlags work_types() const override;
   void record_commands(render_core::FrameResources& frameResources, graphics_api::CommandList &cmdList) override;

private:
   Scene& m_scene;
   ShadowMapRenderer& m_renderer;
};

}