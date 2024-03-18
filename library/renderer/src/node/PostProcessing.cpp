#include "PostProcessing.h"

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

void PostProcessing::record_commands(graphics_api::CommandList &cmdList)
{
   std::array<graphics_api::ClearValue, 3> clearValues{
           graphics_api::ColorPalette::Black,
           graphics_api::DepthStenctilValue{1.0f, 0.0f},
           graphics_api::ColorPalette::Black,
   };
   cmdList.begin_render_pass(m_framebuffers[m_frameIndex], clearValues);

   m_renderer.draw(cmdList, true);

   // m_rectangleRenderer.begin_render(cmdList);
   // m_rectangleRenderer.draw(cmdList, m_renderPass, m_rectangle, m_resolution);

   // m_context2D.begin_render();
   // m_context2D.draw_sprite(m_sprite, {0.0f, 0.0f}, {0.2f, 0.2f});

   // m_textRenderer.begin_render(cmdList);
   // auto textY = 16.0f + m_titleLabel.metric.height;
   // m_textRenderer.draw_text_object(cmdList, m_titleLabel, {16.0f, textY}, {1.0f, 1.0f, 1.0f});
   // textY += 24.0f + m_framerateLabel.metric.height;
   // m_textRenderer.draw_text_object(cmdList, m_framerateLabel, {16.0f, textY}, {1.0f, 1.0f, 1.0f});
   // m_textRenderer.draw_text_object(cmdList, m_framerateValue,
   //                                 {16.0f + m_framerateLabel.metric.width + 8.0f, textY}, {1.0f, 1.0f, 0.4f});
   // textY += 12.0f + m_positionLabel.metric.height;
   // m_textRenderer.draw_text_object(cmdList, m_positionLabel, {16.0f, textY}, {1.0f, 1.0f, 1.0f});
   // m_textRenderer.draw_text_object(cmdList, m_positionValue,
   //                                 {16.0f + m_positionLabel.metric.width + 8.0f, textY}, {1.0f, 1.0f, 0.4f});
   // textY += 12.0f + m_orientationLabel.metric.height;
   // m_textRenderer.draw_text_object(cmdList, m_orientationLabel, {16.0f, textY}, {1.0f, 1.0f, 1.0f});
   // m_textRenderer.draw_text_object(cmdList, m_orientationValue,
   //                                 {16.0f + m_orientationLabel.metric.width + 8.0f, textY},
   //                                 {1.0f, 1.0f, 0.4f});
   // textY += 12.0f + m_triangleCountLabel.metric.height;
   // m_textRenderer.draw_text_object(cmdList, m_triangleCountLabel, {16.0f, textY}, {1.0f, 1.0f, 1.0f});
   // m_textRenderer.draw_text_object(cmdList, m_triangleCountValue,
   //                                 {16.0f + m_triangleCountLabel.metric.width + 8.0f, textY},
   //                                 {1.0f, 1.0f, 0.4f});
   // textY += 12.0f + m_aoLabel.metric.height;
   // m_textRenderer.draw_text_object(cmdList, m_aoLabel, {16.0f, textY}, {1.0f, 1.0f, 1.0f});
   // m_textRenderer.draw_text_object(cmdList, m_aoValue, {16.0f + m_aoLabel.metric.width + 8.0f, textY},
   //                                 {1.0f, 1.0f, 0.4f});
   // textY += 12.0f + m_aaLabel.metric.height;
   // m_textRenderer.draw_text_object(cmdList, m_aaLabel, {16.0f, textY}, {1.0f, 1.0f, 1.0f});
   // m_textRenderer.draw_text_object(cmdList, m_aaValue, {16.0f + m_aaLabel.metric.width + 8.0f, textY},
   //                                 {1.0f, 1.0f, 0.4f});

   cmdList.end_render_pass();
}

void PostProcessing::set_index(const u32 index)
{
   m_frameIndex = index;
}

}// namespace triglav::renderer::node