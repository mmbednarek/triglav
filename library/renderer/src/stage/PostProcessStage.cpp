#include "stage/PostProcessStage.hpp"

#include "triglav/render_core/BuildContext.hpp"

namespace triglav::renderer::stage {

struct PostProcessingPushConstants
{
   int enableFXAA{};
   int hideUI{};
   int bloomEnabled{};
};

using namespace name_literals;

void PostProcessStage::build_stage(render_core::BuildContext& ctx) const
{
   ctx.declare_render_target("ui.target"_name);

   ctx.begin_render_pass("ui.pass"_name, "ui.target"_name);
   ctx.clear_color("ui.target"_name, {0, 0, 0, 0});
   ctx.end_render_pass();

   ctx.declare_render_target("core.color_out"_name, GAPI_FORMAT(BGRA, sRGB));

   ctx.begin_render_pass("post_processing"_name, "core.color_out"_name);

   ctx.bind_fragment_shader("post_processing.fshader"_rc);

   ctx.push_constant(PostProcessingPushConstants{
      .enableFXAA = 1,
      .hideUI = 0,
      .bloomEnabled = 1,
   });

   ctx.bind_samplable_texture(0, "shading.color"_name);
   ctx.bind_samplable_texture(1, "shading.blurred_bloom"_name);
   ctx.bind_samplable_texture(2, "ui.target"_name);

   ctx.set_is_blending_enabled(false);

   ctx.draw_full_screen_quad();

   ctx.end_render_pass();

   ctx.export_texture("core.color_out"_name, graphics_api::PipelineStage::Transfer, graphics_api::TextureState::TransferSrc,
                      graphics_api::TextureUsage::TransferSrc);
}

}// namespace triglav::renderer::stage