#include "DefaultRenderOverlay.hpp"

#include "triglav/render_core/BuildContext.hpp"

namespace triglav::editor {

using namespace name_literals;

void DefaultRenderOverlay::build_update_job(render_core::BuildContext& ctx)
{
   ctx.init_buffer("default_render_overlay.dump_buffer"_name, Vector4{1, 0, 0, 0});
}

void DefaultRenderOverlay::build_render_job(render_core::BuildContext& ctx)
{
   ctx.declare_render_target("render_viewport.out"_name, GAPI_FORMAT(BGRA, sRGB));

   ctx.begin_render_pass("editor_tools"_name, "render_viewport.out"_name);
   ctx.clear_color("render_viewport.out"_name, {0, 0, 0, 1});
   ctx.end_render_pass();

   ctx.export_texture("render_viewport.out"_name, graphics_api::PipelineStage::Transfer, graphics_api::TextureState::TransferSrc,
                      graphics_api::TextureUsage::TransferSrc);
}

void DefaultRenderOverlay::update(render_core::JobGraph& /*graph*/, u32 /*frame_index*/, float /*delta_time*/)
{
   // nothing to do
}

void DefaultRenderOverlay::set_dimensions(const Vector4 dimensions)
{
   m_dimensions = dimensions;
}

Vector4 DefaultRenderOverlay::dimensions() const
{
   return m_dimensions;
}

}// namespace triglav::editor
