#include "stage/PostProcessStage.hpp"

#include "UpdateUserInterfaceJob.hpp"

#include "triglav/render_core/BuildContext.hpp"

#include <Config.hpp>

namespace triglav::renderer::stage {

struct PostProcessingPushConstants
{
   int enable_fxaa{};
   int bloom_enabled{};
};

using namespace name_literals;

PostProcessStage::PostProcessStage(UpdateUserInterfaceJob* update_user_interface_job, Name output_render_target) :
    m_update_user_interface_job(update_user_interface_job),
    m_output_render_target(output_render_target)
{
}

void PostProcessStage::build_stage(render_core::BuildContext& ctx, const Config& config) const
{
   ctx.declare_render_target(m_output_render_target, GAPI_FORMAT(BGRA, sRGB));

   ctx.begin_render_pass("post_processing"_name, m_output_render_target);

   ctx.bind_fragment_shader("shader/pass/post_processing.fshader"_rc);

   ctx.push_constant(PostProcessingPushConstants{
      .enable_fxaa = config.antialiasing == AntialiasingMethod::FastApproximate,
      .bloom_enabled = config.is_bloom_enabled,
   });

   ctx.bind_samplable_texture(0, "shading.color"_name);
   ctx.bind_samplable_texture(1, "shading.blurred_bloom"_name);

   ctx.set_is_blending_enabled(false);

   ctx.draw_full_screen_quad();

   if (m_update_user_interface_job != nullptr && !config.is_uihidden) {
      m_update_user_interface_job->render_ui(ctx);
   }

   ctx.end_render_pass();

   ctx.export_texture(m_output_render_target, graphics_api::PipelineStage::Transfer, graphics_api::TextureState::TransferSrc,
                      graphics_api::TextureUsage::TransferSrc);
}

}// namespace triglav::renderer::stage