#include "Shading.h"

namespace triglav::renderer::node {

using graphics_api::AttachmentAttribute;
using graphics_api::SampleCount;

using namespace name_literals;

Shading::Shading(graphics_api::Device &device, resource::ResourceManager &resourceManager, Scene &scene) :
    m_shadingRenderTarget(GAPI_CHECK(graphics_api::RenderTargetBuilder(device)
                               .attachment("shading"_name,
                                           AttachmentAttribute::Color | AttachmentAttribute::ClearImage |
                                                   AttachmentAttribute::StoreImage,
                                           GAPI_FORMAT(RGBA, Float16), SampleCount::Single)
                               .attachment("bloom"_name,
                                           AttachmentAttribute::Color | AttachmentAttribute::ClearImage |
                                           AttachmentAttribute::StoreImage | AttachmentAttribute::TransferSrc,
                                           GAPI_FORMAT(RGBA, Float16), SampleCount::Single)
                               .build())),
   m_shadingRenderer(device, m_shadingRenderTarget, resourceManager),
   m_scene(scene)
{
}

std::unique_ptr<render_core::NodeFrameResources> Shading::create_node_resources()
{
   auto result = IRenderNode::create_node_resources();
   result->add_render_target("shading"_name, m_shadingRenderTarget);
   return result;
}

graphics_api::WorkTypeFlags Shading::work_types() const
{
   return graphics_api::WorkType::Graphics;
}

void Shading::record_commands(render_core::FrameResources &frameResources,
                              render_core::NodeFrameResources &resources, graphics_api::CommandList &cmdList)
{
   std::array<graphics_api::ClearValue, 2> clearValues{
           graphics_api::ColorPalette::Black,
           graphics_api::ColorPalette::Black,
   };
   cmdList.begin_render_pass(resources.framebuffer("shading"_name), clearValues);

   const auto shadowMat = m_scene.shadow_map_camera().view_projection_matrix() *
                          glm::inverse(m_scene.camera().view_matrix());
   const auto lightPosition =
           m_scene.camera().view_matrix() * glm::vec4(m_scene.shadow_map_camera().position(), 1.0);

   m_shadingRenderer.draw(frameResources, cmdList, glm::vec3(lightPosition), shadowMat);

   cmdList.end_render_pass();
}

}// namespace triglav::renderer::node
