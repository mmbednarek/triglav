#include "Util.hpp"

#include "triglav/render_core/BuildContext.hpp"

namespace triglav::renderer {

using namespace name_literals;

void blur_texture(render_core::BuildContext& ctx, const Name srcTexture, const Name dstTexture, const graphics_api::ColorFormat dstFormat)
{
   ctx.declare_screen_size_texture(dstTexture, dstFormat);

   if (dstFormat.channel_count() == 1) {
      ctx.bind_compute_shader("blur/sc.cshader"_rc);
   } else {
      ctx.bind_compute_shader("blur.cshader"_rc);
   }

   ctx.bind_samplable_texture(0, srcTexture);
   ctx.bind_rw_texture(1, dstTexture);

   ctx.dispatch({divide_rounded_up(ctx.screen_size().x, 16), divide_rounded_up(ctx.screen_size().y, 16), 1});
}

}// namespace triglav::renderer