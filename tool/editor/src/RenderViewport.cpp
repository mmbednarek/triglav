#include "RenderViewport.hpp"

namespace triglav::editor {

using namespace name_literals;

void RenderViewport::initialize()
{
   auto& render_job = m_jobGraph.add_job("viewport_render"_name);
   this->build_render_job(render_job);
}

void RenderViewport::update() {}

void RenderViewport::build_render_job(render_core::BuildContext& ctx)
{
   ctx.declare_render_target("render_viewport.out"_name, GAPI_FORMAT(RGBA, UNorm8));

   ctx.begin_render_pass("viewport_render"_name, "render_viewport.out"_name);
   ctx.clear_color("render_viewport.out"_name, {1, 0, 1, 1});
   ctx.end_render_pass();
}

}// namespace triglav::editor
