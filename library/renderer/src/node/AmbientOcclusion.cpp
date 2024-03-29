#include "AmbientOcclusion.h"

#include <triglav/render_core/FrameResources.h>

namespace triglav::renderer::node {

AmbientOcclusion::AmbientOcclusion(graphics_api::Framebuffer &ambientOcclusionFramebuffer,
                                   AmbientOcclusionRenderer &renderer, Scene &scene) :
    m_ambientOcclusionFramebuffer(ambientOcclusionFramebuffer),
    m_renderer(renderer),
    m_scene(scene)
{
}

graphics_api::WorkTypeFlags AmbientOcclusion::work_types() const
{
   return graphics_api::WorkType::Graphics;
}

void AmbientOcclusion::record_commands(render_core::FrameResources& frameResources, graphics_api::CommandList &cmdList)
{
   std::array<graphics_api::ClearValue, 1> clearValues{
           graphics_api::ColorPalette::Black,
   };
   cmdList.begin_render_pass(m_ambientOcclusionFramebuffer, clearValues);

   m_renderer.draw(cmdList, m_scene.camera().projection_matrix());

   cmdList.end_render_pass();
}

}// namespace triglav::renderer::node
