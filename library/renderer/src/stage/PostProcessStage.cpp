#include "stage/PostProcessStage.hpp"

#include "triglav/render_core/BuildContext.hpp"

namespace triglav::renderer::stage {

using namespace name_literals;

void PostProcessStage::build_stage(render_core::BuildContext& ctx) const
{
   ctx.declare_render_target("core.color_out"_name);

   ctx.begin_render_pass("post_processing"_name, "core.color_out"_name);
   ctx.end_render_pass();
}

}// namespace triglav::renderer::stage