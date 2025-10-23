#include "RenderViewport.hpp"

namespace triglav::editor {

using namespace name_literals;

RenderViewport::RenderViewport(LevelEditor& levelEditor, const Vector4 dimensions) :
    m_levelEditor(levelEditor),
    m_dimensions(dimensions)
{
}

void RenderViewport::build_update_job(render_core::BuildContext& ctx)
{
   m_levelEditor.m_updateViewParamsJob.build_job(ctx);
}

void RenderViewport::build_render_job(render_core::BuildContext& ctx)
{
   // ctx.declare_render_target("render_viewport.out"_name, GAPI_FORMAT(RGBA, UNorm8));

   // ctx.begin_render_pass("viewport_render"_name, "render_viewport.out"_name);
   // ctx.clear_color("render_viewport.out"_name, {1, 0, 1, 1});
   // ctx.end_render_pass();

   ctx.declare_screen_size_texture("render_viewport.out"_name, GAPI_FORMAT(BGRA, sRGB));

   m_levelEditor.m_renderingJob.build_job(ctx);

   ctx.blit_texture("shading.color"_name, "render_viewport.out"_name);

   ctx.export_texture("render_viewport.out"_name, graphics_api::PipelineStage::Transfer, graphics_api::TextureState::TransferSrc,
                      graphics_api::TextureUsage::TransferSrc);
}

void RenderViewport::update(render_core::JobGraph& graph, u32 frameIndex, float deltaTime)
{
   m_levelEditor.m_updateViewParamsJob.prepare_frame(graph, frameIndex, deltaTime);
}

[[nodiscard]] Vector4 RenderViewport::dimensions() const
{
   return m_dimensions;
}

}// namespace triglav::editor
