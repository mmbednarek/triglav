#include "ShadowMap.h"

namespace triglav::renderer::node {

using namespace name_literals;
using graphics_api::AttachmentAttribute;

constexpr auto g_shadowMapResolution = graphics_api::Resolution{4096, 4096};
constexpr auto g_shadowMapFormat     = GAPI_FORMAT(D, Float32);

ShadowMap::ShadowMap(graphics_api::Device &device, resource::ResourceManager &resourceManager, Scene &scene) :
    m_depthRenderTarget(
            GAPI_CHECK(graphics_api::RenderTargetBuilder(device)
                               .attachment("sm"_name_id,
                                           AttachmentAttribute::Depth | AttachmentAttribute::ClearImage |
                                                   AttachmentAttribute::StoreImage,
                                           g_shadowMapFormat)
                               .build())),
    m_renderer(device, resourceManager, m_depthRenderTarget),
    m_scene(scene)
{
   m_scene.set_shadow_map_renderer(&m_renderer);
}

std::unique_ptr<render_core::NodeFrameResources> ShadowMap::create_node_resources()
{
   auto result = IRenderNode::create_node_resources();
   result->add_render_target_with_resolution("sm"_name_id, m_depthRenderTarget, g_shadowMapResolution);
   return result;
}

graphics_api::WorkTypeFlags ShadowMap::work_types() const
{
   return graphics_api::WorkType::Graphics;
}

void ShadowMap::record_commands(render_core::FrameResources &frameResources,
                                render_core::NodeFrameResources &resources,
                                graphics_api::CommandList &cmdList)
{
   std::array<graphics_api::ClearValue, 1> clearValues{
           graphics_api::ClearValue{graphics_api::DepthStenctilValue{1.0f, 0}},
   };

   cmdList.begin_render_pass(resources.framebuffer("sm"_name_id), clearValues);

   m_renderer.on_begin_render(cmdList);
   m_scene.render_shadow_map(cmdList);

   cmdList.end_render_pass();
}

}// namespace triglav::renderer::node
