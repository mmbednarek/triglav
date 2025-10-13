#include "UpdateUserInterfaceJob.hpp"

#include "triglav/render_core/BuildContext.hpp"

namespace triglav::renderer {

using namespace triglav::name_literals;

UpdateUserInterfaceJob::UpdateUserInterfaceJob(graphics_api::Device& device, render_core::GlyphCache& glyphCache,
                                               ui_core::Viewport& viewport, resource::ResourceManager& resourceManager) :
    m_device(device),
    m_viewport(viewport),
    m_rectangleRenderer(m_device, viewport),
    m_spriteRenderer(m_device, viewport, resourceManager),
    m_textRenderer(m_device, glyphCache, viewport)
{
}

void UpdateUserInterfaceJob::build_job(render_core::BuildContext& ctx) const
{
   // Init viewport info
   ctx.init_buffer("ui.viewport_info"_name, Vector2{m_viewport.dimensions()});

   m_rectangleRenderer.build_data_update(ctx);
   m_spriteRenderer.build_data_update(ctx);
   m_textRenderer.build_data_preparation(ctx);

   ctx.export_buffer("ui.viewport_info"_name, graphics_api::PipelineStage::VertexShader, graphics_api::BufferAccess::ShaderRead,
                     graphics_api::BufferUsage::UniformBuffer);
}

void UpdateUserInterfaceJob::prepare_frame(render_core::JobGraph& graph, const u32 frameIndex)
{
   m_rectangleRenderer.prepare_frame(graph, frameIndex);
   m_spriteRenderer.prepare_frame(graph, frameIndex);
   m_textRenderer.prepare_frame(graph, frameIndex);
}

void UpdateUserInterfaceJob::render_ui(render_core::BuildContext& ctx)
{

   m_rectangleRenderer.build_render_ui(ctx);
   m_spriteRenderer.build_render_ui(ctx);
   m_textRenderer.build_render_ui(ctx);
}

}// namespace triglav::renderer
