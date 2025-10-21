#include "RenderViewport.hpp"

namespace triglav::editor {

using namespace name_literals;

RenderViewport::RenderViewport(const Vector4 dimensions) :
    m_dimensions(dimensions)
{
}

void RenderViewport::initialize(render_core::JobGraph& jobGraph)
{
   auto& render_job = jobGraph.add_job("viewport_render"_name);
   this->build_render_job(render_job);
}

void RenderViewport::update() {}

void RenderViewport::build_render_job(render_core::BuildContext& ctx)
{
   ctx.declare_sized_render_target("render_viewport.out"_name, rect_size(m_dimensions), GAPI_FORMAT(RGBA, UNorm8));
   ctx.begin_render_pass("viewport_render"_name, "render_viewport.out"_name);
   ctx.clear_color("render_viewport.out"_name, {1, 0, 1, 1});
   ctx.end_render_pass();

   ctx.copy_texture_region("render_viewport.out"_name, {0, 0}, "core.color_out"_name, rect_position(m_dimensions), rect_size(m_dimensions));
}

}// namespace triglav::editor
