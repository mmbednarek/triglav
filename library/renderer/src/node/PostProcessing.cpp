#include "PostProcessing.h"

#include "triglav/graphics_api/Framebuffer.h"

namespace triglav::renderer::node {

using namespace name_literals;
using graphics_api::AttachmentAttribute;

PostProcessing::PostProcessing(graphics_api::Device& device, resource::ResourceManager& resourceManager,
                               graphics_api::RenderTarget& renderTarget, std::vector<graphics_api::Framebuffer>& framebuffers) :
    m_renderer(device, renderTarget, resourceManager),
    m_framebuffers(framebuffers)
{
}

graphics_api::WorkTypeFlags PostProcessing::work_types() const
{
   return graphics_api::WorkType::Graphics;
}

void PostProcessing::record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& resources,
                                     graphics_api::CommandList& cmdList)
{
   std::array<graphics_api::ClearValue, 2> clearValues{
      {{graphics_api::ColorPalette::Black}, {graphics_api::DepthStenctilValue{1.0f, 0}}},
   };

   cmdList.begin_render_pass(m_framebuffers[m_frameIndex], clearValues);

   m_renderer.draw(frameResources, cmdList);

   cmdList.end_render_pass();
}

void PostProcessing::set_index(const u32 index)
{
   m_frameIndex = index;
}

}// namespace triglav::renderer::node