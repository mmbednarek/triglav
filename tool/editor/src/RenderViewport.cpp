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
   m_levelEditor.m_renderingJob.build_job(ctx);
   // ctx.declare_sized_render_target("render_viewport.out"_name, rect_size(m_dimensions), GAPI_FORMAT(RGBA, UNorm8));
   // ctx.begin_render_pass("viewport_render"_name, "render_viewport.out"_name);
   // ctx.clear_color("render_viewport.out"_name, {1, 0, 1, 1});
   // ctx.end_render_pass();

   ctx.copy_texture_region("shading.color"_name, {0, 0}, "core.color_out"_name, rect_position(m_dimensions), rect_size(m_dimensions));
}

}// namespace triglav::editor
