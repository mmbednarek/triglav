#include "UpdateUserInterfaceJob.hpp"

#include "triglav/render_core/BuildContext.hpp"

namespace triglav::renderer {

using namespace triglav::name_literals;

UpdateUserInterfaceJob::UpdateUserInterfaceJob(graphics_api::Device& device, render_core::GlyphCache& glyph_cache,
                                               ui_core::Viewport& viewport, resource::ResourceManager& resource_manager,
                                               render_core::IRenderer& renderer) :
    m_device(device),
    m_viewport(viewport),
    m_rectangle_renderer(viewport),
    m_sprite_renderer(viewport, resource_manager),
    m_text_renderer(m_device, glyph_cache, viewport, renderer)
{
}

void UpdateUserInterfaceJob::build_job(render_core::BuildContext& ctx) const
{
   // Init viewport info
   ctx.init_buffer("ui.viewport_info"_name, Vector2{m_viewport.dimensions()});

   m_rectangle_renderer.build_data_update(ctx);
   m_sprite_renderer.build_data_update(ctx);
   m_text_renderer.build_data_preparation(ctx);

   ctx.export_buffer("ui.viewport_info"_name, graphics_api::PipelineStage::VertexShader, graphics_api::BufferAccess::ShaderRead,
                     graphics_api::BufferUsage::UniformBuffer);
}

void UpdateUserInterfaceJob::prepare_frame(render_core::JobGraph& graph, const u32 frame_index)
{
   m_rectangle_renderer.prepare_frame(graph, frame_index);
   m_sprite_renderer.prepare_frame(graph, frame_index);
   m_text_renderer.prepare_frame(graph, frame_index);
}

void UpdateUserInterfaceJob::render_ui(render_core::BuildContext& ctx)
{

   m_rectangle_renderer.build_render_ui(ctx);
   m_sprite_renderer.build_render_ui(ctx);
   m_text_renderer.build_render_ui(ctx);
}

}// namespace triglav::renderer
