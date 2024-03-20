#pragma once

#include "triglav/render_core/IRenderNode.hpp"

#include "Scene.h"
#include "SkyBox.h"

#include <GroundRenderer.h>

namespace triglav::renderer::node {

class Geometry : public render_core::IRenderNode
{
public:
   Geometry(Scene &scene, SkyBox& skybox, graphics_api::Framebuffer &modelFramebuffer, GroundRenderer &groundRenderer,
           ModelRenderer &modelRenderer);

  [[nodiscard]] graphics_api::WorkTypeFlags work_types() const override;
   void record_commands(graphics_api::CommandList &cmdList) override;

private:
   Scene& m_scene;
   SkyBox& m_skybox;
   graphics_api::Framebuffer& m_modelFramebuffer;
   GroundRenderer &m_groundRenderer;
   ModelRenderer &m_modelRenderer;
};

}
