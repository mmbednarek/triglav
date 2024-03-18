#include "ShadowMap.h"

namespace triglav::renderer::node {

ShadowMap::ShadowMap(Scene &scene, ShadowMapRenderer &renderer) :
    m_scene(scene),
    m_renderer(renderer)
{
}

graphics_api::WorkTypeFlags ShadowMap::work_types() const
{
   return graphics_api::WorkType::Graphics;
}

void ShadowMap::record_commands(graphics_api::CommandList &cmdList)
{
   std::array<graphics_api::ClearValue, 1> clearValues{
           graphics_api::DepthStenctilValue{1.0f, 0.0f}
   };
   cmdList.begin_render_pass(m_renderer.framebuffer(), clearValues);

   m_renderer.on_begin_render(cmdList);
   m_scene.render_shadow_map(cmdList);

   cmdList.end_render_pass();
}

}// namespace triglav::renderer::node
