#include "PostProcessing.h"

#include "triglav/graphics_api/Framebuffer.h"

namespace triglav::renderer::node {

PostProcessing::PostProcessing(PostProcessingRenderer &renderer,
                               std::vector<graphics_api::Framebuffer> &framebuffers) :
    m_renderer(renderer),
    m_framebuffers(framebuffers)
{
}

graphics_api::WorkTypeFlags PostProcessing::work_types() const
{
   return graphics_api::WorkType::Graphics;
}

void PostProcessing::record_commands(render_core::FrameResources &frameResources,
                                     render_core::NodeFrameResources &resources,
                                     graphics_api::CommandList &cmdList)
{
   std::array<graphics_api::ClearValue, 2> clearValues{
           graphics_api::ColorPalette::Black,
           graphics_api::DepthStenctilValue{1.0f, 0},
   };
   cmdList.begin_render_pass(m_framebuffers[m_frameIndex], clearValues);

   m_renderer.draw(frameResources, cmdList, true);

   cmdList.end_render_pass();
}

void PostProcessing::set_index(const u32 index)
{
   m_frameIndex = index;
}

}// namespace triglav::renderer::node