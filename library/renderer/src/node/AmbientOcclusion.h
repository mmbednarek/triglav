#pragma once

#include "triglav/graphics_api/Framebuffer.h"
#include "triglav/render_core/IRenderNode.hpp"

#include "AmbientOcclusionRenderer.h"
#include "Scene.h"

namespace triglav::renderer::node {

class AmbientOcclusion : public render_core::IRenderNode
{
public:
   AmbientOcclusion(graphics_api::Framebuffer &ambientOcclusionFramebuffer, AmbientOcclusionRenderer &renderer,
                   Scene &scene);

  [[nodiscard]] graphics_api::WorkTypeFlags work_types() const override;
   void record_commands(render_core::FrameResources &frameResources,
                       graphics_api::CommandList &cmdList) override;

private:
   graphics_api::Framebuffer& m_ambientOcclusionFramebuffer;
   AmbientOcclusionRenderer& m_renderer;
   Scene& m_scene;
};

}
