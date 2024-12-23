#include "AmbientOcclusion.hpp"

#include "triglav/render_core/FrameResources.hpp"

namespace triglav::renderer::node {

using namespace name_literals;
using graphics_api::AttachmentAttribute;
using graphics_api::SampleCount;

AmbientOcclusion::AmbientOcclusion(graphics_api::Device& device, resource::ResourceManager& resourceManager, Scene& scene) :
    m_renderTarget(
       GAPI_CHECK(graphics_api::RenderTargetBuilder(device)
                     .attachment("ao"_name, AttachmentAttribute::Color | AttachmentAttribute::ClearImage | AttachmentAttribute::StoreImage,
                                 GAPI_FORMAT(R, Float16), SampleCount::Single)
                     .build())),
    m_renderer(device, m_renderTarget, resourceManager, resourceManager.get("noise.tex"_rc)),
    m_scene(scene)
{
}

std::unique_ptr<render_core::NodeFrameResources> AmbientOcclusion::create_node_resources()
{
   auto result = std::make_unique<render_core::NodeFrameResources>();
   result->add_render_target("ao"_name, m_renderTarget);
   return result;
}

graphics_api::WorkTypeFlags AmbientOcclusion::work_types() const
{
   return graphics_api::WorkType::Graphics;
}

void AmbientOcclusion::record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& resources,
                                       graphics_api::CommandList& cmdList)
{
   if (frameResources.get_option<AmbientOcclusionMethod>("ao_method"_name) != AmbientOcclusionMethod::ScreenSpace)
      return;

   std::array<graphics_api::ClearValue, 1> clearValues{
      graphics_api::ColorPalette::Black,
   };

   cmdList.begin_render_pass(resources.framebuffer("ao"_name), clearValues);

   m_renderer.draw(frameResources, cmdList, m_scene.camera().projection_matrix());

   cmdList.end_render_pass();
}

}// namespace triglav::renderer::node
