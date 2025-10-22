#include "stage/PostProcessStage.hpp"

#include "UpdateUserInterfaceJob.hpp"

#include "triglav/render_core/BuildContext.hpp"

#include <Config.hpp>

namespace triglav::renderer::stage {

struct PostProcessingPushConstants
{
   int enableFXAA{};
   int bloomEnabled{};
};

using namespace name_literals;

PostProcessStage::PostProcessStage(UpdateUserInterfaceJob* updateUserInterfaceJob) :
    m_updateUserInterfaceJob(updateUserInterfaceJob)
{
}

void PostProcessStage::build_stage(render_core::BuildContext& ctx, const Config& config) const
{
   ctx.declare_render_target("core.color_out"_name, GAPI_FORMAT(BGRA, sRGB));

   ctx.begin_render_pass("post_processing"_name, "core.color_out"_name);

   ctx.bind_fragment_shader("post_processing.fshader"_rc);

   ctx.push_constant(PostProcessingPushConstants{
      .enableFXAA = config.antialiasing == AntialiasingMethod::FastApproximate,
      .bloomEnabled = config.isBloomEnabled,
   });

   ctx.bind_samplable_texture(0, "shading.color"_name);
   ctx.bind_samplable_texture(1, "shading.blurred_bloom"_name);

   ctx.set_is_blending_enabled(false);

   ctx.draw_full_screen_quad();

   if (m_updateUserInterfaceJob != nullptr && !config.isUIHidden) {
      m_updateUserInterfaceJob->render_ui(ctx);
   }

   ctx.end_render_pass();

   ctx.export_texture("core.color_out"_name, graphics_api::PipelineStage::Transfer, graphics_api::TextureState::TransferSrc,
                      graphics_api::TextureUsage::TransferSrc);
}

}// namespace triglav::renderer::stage