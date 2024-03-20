#include "Geometry.h"

namespace triglav::renderer::node {

Geometry::Geometry(Scene &scene, SkyBox& skybox, graphics_api::Framebuffer &modelFramebuffer, GroundRenderer &groundRenderer,
                   ModelRenderer &modelRenderer) :
    m_scene(scene),
    m_skybox(skybox),
    m_modelFramebuffer(modelFramebuffer),
    m_groundRenderer(groundRenderer),
    m_modelRenderer(modelRenderer)
{
}

graphics_api::WorkTypeFlags Geometry::work_types() const
{
   return graphics_api::WorkType::Graphics;
}

void Geometry::record_commands(graphics_api::CommandList &cmdList)
{
      std::array<graphics_api::ClearValue, 4> clearValues{
              graphics_api::ColorPalette::Black,
              graphics_api::ColorPalette::Black,
              graphics_api::ColorPalette::Black,
              graphics_api::DepthStenctilValue{1.0f, 0.0f},
      };
      cmdList.begin_render_pass(m_modelFramebuffer, clearValues);

      m_skybox.on_render(cmdList, m_scene.yaw(), m_scene.pitch(), static_cast<float>(m_modelFramebuffer.resolution().width),
                         static_cast<float>(m_modelFramebuffer.resolution().height));

      m_groundRenderer.draw(cmdList, m_scene.camera());

      m_modelRenderer.begin_render(cmdList);
      m_scene.render(cmdList);

      // if (m_showDebugLines) {
      //    m_debugLinesRenderer.begin_render(m_commandList);
      //    m_scene.render_debug_lines();
      // }

      cmdList.end_render_pass();
}

}// namespace triglav::renderer::node
