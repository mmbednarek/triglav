#include "UpdateUserInterfaceJob.hpp"

namespace triglav::renderer {

UpdateUserInterfaceJob::UpdateUserInterfaceJob(graphics_api::Device& device, render_core::GlyphCache& glyphCache,
                                               ui_core::Viewport& viewport, resource::ResourceManager& resourceManager) :
    m_device(device),
    m_rectangleRenderer(m_device, viewport),
    m_spriteRenderer(m_device, viewport, resourceManager),
    m_textRenderer(m_device, glyphCache, viewport)
{
}

void UpdateUserInterfaceJob::build_job(render_core::BuildContext& ctx) const
{
   m_textRenderer.build_data_preparation(ctx);
}

void UpdateUserInterfaceJob::prepare_frame(render_core::JobGraph& graph, const u32 frameIndex)
{
   m_textRenderer.prepare_frame(graph, frameIndex);
}

void UpdateUserInterfaceJob::render_ui(render_core::BuildContext& ctx)
{
   m_rectangleRenderer.build_render_ui(ctx);
   m_spriteRenderer.build_render_ui(ctx);
   m_textRenderer.build_render_ui(ctx);
}

}// namespace triglav::renderer
