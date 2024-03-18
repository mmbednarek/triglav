#include "Shading.h"

namespace triglav::renderer::node {

Shading::Shading(graphics_api::Framebuffer &framebuffer, ShadingRenderer &shadingRenderer, Scene &scene) :
    m_framebuffer(framebuffer),
    m_shadingRenderer(shadingRenderer),
    m_scene(scene)
{
}

graphics_api::WorkTypeFlags Shading::work_types() const
{
   return graphics_api::WorkType::Graphics;
}

void Shading::record_commands(graphics_api::CommandList &cmdList)
{
   std::array<graphics_api::ClearValue, 1> clearValues{
           graphics_api::ColorPalette::Black,
   };
   cmdList.begin_render_pass(m_framebuffer, clearValues);

   const auto shadowMat = m_scene.shadow_map_camera().view_projection_matrix() *
                          glm::inverse(m_scene.camera().view_matrix());
   const auto lightPosition =
           m_scene.camera().view_matrix() * glm::vec4(m_scene.shadow_map_camera().position(), 1.0);

   m_shadingRenderer.draw(cmdList, glm::vec3(lightPosition), shadowMat, true);

   cmdList.end_render_pass();
}

}// namespace triglav::renderer::node
