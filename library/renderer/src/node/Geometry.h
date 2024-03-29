#pragma once

#include "triglav/render_core/IRenderNode.hpp"

#include "Scene.h"
#include "SkyBox.h"

#include <GroundRenderer.h>

namespace triglav::renderer::node {

class Geometry : public render_core::IRenderNode
{
public:
   Geometry(graphics_api::Device& device, Scene &scene, SkyBox& skybox, graphics_api::Framebuffer &modelFramebuffer, GroundRenderer &groundRenderer,
           ModelRenderer &modelRenderer);

  [[nodiscard]] graphics_api::WorkTypeFlags work_types() const override;
   void record_commands(render_core::FrameResources& frameResources, graphics_api::CommandList &cmdList) override;

   [[nodiscard]] float gpu_time() const;

private:
   Scene& m_scene;
   SkyBox& m_skybox;
   graphics_api::Framebuffer& m_modelFramebuffer;
   GroundRenderer &m_groundRenderer;
   ModelRenderer &m_modelRenderer;
   graphics_api::TimestampArray m_timestampArray;
};

}
