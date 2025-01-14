#include "stage/ShadingStage.hpp"

#include "triglav/render_core/BuildContext.hpp"

namespace triglav::renderer::stage {

using namespace name_literals;
using namespace render_core::literals;

struct PushConstants
{
   Vector4 lightPosition;
   int isAmbientOcclusionEnabled;
   int shouldSampleShadows;
};

void ShadingStage::build_stage(render_core::BuildContext& ctx) const
{
   ctx.declare_screen_size_texture("ray_tracing.shadow_texture"_name, GAPI_FORMAT(R, UNorm8));

   ctx.declare_render_target("shading.color"_name, GAPI_FORMAT(RGBA, Float16));
   ctx.declare_render_target("shading.bloom"_name, GAPI_FORMAT(RGBA, Float16));
   ctx.declare_depth_target("shading.depth"_name, GAPI_FORMAT(D, UNorm16));

   ctx.begin_render_pass("shading"_name, "shading.color"_name, "shading.bloom"_name, "shading.depth"_name);

   ctx.bind_fragment_shader("shading.fshader"_rc);

   ctx.bind_samplable_texture(0, "gbuffer.albedo"_name);
   ctx.bind_samplable_texture(1, "gbuffer.position"_name);
   ctx.bind_samplable_texture(2, "gbuffer.normal"_name);
   ctx.bind_samplable_texture(3, "ambient_occlusion.target"_name);

   std::array<render_core::TextureRef, 3> shadowTextures{"shadow_map.cascade0"_name, "shadow_map.cascade1"_name,
                                                         "shadow_map.cascade2"_name};
   ctx.bind_sampled_texture_array(4, shadowTextures);

   ctx.bind_samplable_texture(5, "ray_tracing.shadow_texture"_name);
   ctx.bind_uniform_buffer(6, "core.view_properties"_external);
   ctx.bind_uniform_buffer(7, "shadow_map.matrices"_external);

   ctx.push_constant(PushConstants{
      glm::vec4(-30, 0, -5, 1.0),
      true,
      false,
   });

   ctx.set_is_blending_enabled(false);

   ctx.draw_full_screen_quad();

   ctx.end_render_pass();
}

}// namespace triglav::renderer::stage