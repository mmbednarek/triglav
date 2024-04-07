#pragma once

#include "triglav/render_core/IRenderNode.hpp"

#include "Scene.h"
#include "ShadingRenderer.h"

namespace triglav::renderer::node {

class Shading : public render_core::IRenderNode
{
 public:
   Shading(graphics_api::Framebuffer &framebuffer, ShadingRenderer &shadingRenderer, Scene &scene);

   [[nodiscard]] graphics_api::WorkTypeFlags work_types() const override;
   void record_commands(render_core::FrameResources &frameResources,
                        render_core::NodeFrameResources &resources,
                        graphics_api::CommandList &cmdList) override;

 private:
   graphics_api::Framebuffer &m_framebuffer;
   ShadingRenderer &m_shadingRenderer;
   Scene &m_scene;
};

}// namespace triglav::renderer::node
