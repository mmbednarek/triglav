#include "stage/ShadingStage.hpp"

#include "Util.hpp"

#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/JobGraph.hpp"

#include <Config.hpp>
#include <random>

namespace triglav::renderer::stage {

using namespace name_literals;
using namespace render_core::literals;

struct Particle
{
   alignas(16) glm::vec3 position;
   alignas(16) glm::vec3 velocity;
   float animation;
   float rotation;
   float angularVelocity;
   float scale;
};

constexpr auto g_particleCount = 256;

struct ShadingPushConstants
{
   alignas(16) Vector4 lightPosition;
   int isAmbientOcclusionEnabled;
   int shouldSampleShadows;
};

void ShadingStage::build_stage(render_core::BuildContext& ctx, const Config& config) const
{
   if (!ctx.is_ray_tracing_supported()) {
      // declare dummy shadows texture
      ctx.declare_screen_size_texture("ray_tracing.shadows"_name, GAPI_FORMAT(R, UNorm8));
   }

   ctx.declare_render_target("shading.color"_name, GAPI_FORMAT(RGBA, Float16));
   ctx.declare_render_target("shading.bloom"_name, GAPI_FORMAT(RGBA, Float16));
   ctx.declare_depth_target("shading.depth"_name, GAPI_FORMAT(D, UNorm16));

   this->prepare_particles(ctx);

   ctx.copy_texture("gbuffer.depth"_name, "shading.depth"_name);

   ctx.begin_render_pass("shading"_name, "shading.color"_name, "shading.bloom"_name, "shading.depth"_name);

   ctx.bind_fragment_shader("shading.fshader"_rc);

   ctx.bind_samplable_texture(0, "gbuffer.albedo"_name);
   ctx.bind_samplable_texture(1, "gbuffer.position"_name);
   ctx.bind_samplable_texture(2, "gbuffer.normal"_name);
   if (config.ambientOcclusion == AmbientOcclusionMethod::RayTraced) {
      ctx.bind_samplable_texture(3, "ray_tracing.ambient_occlusion.blurred"_name);
   } else {
      ctx.bind_samplable_texture(3, "ambient_occlusion.blurred"_name);
   }

   std::array<render_core::TextureRef, 3> shadowTextures{"shadow_map.cascade0"_name, "shadow_map.cascade1"_name,
                                                         "shadow_map.cascade2"_name};
   ctx.bind_sampled_texture_array(4, shadowTextures);

   ctx.bind_samplable_texture(5, "ray_tracing.shadows"_name);
   ctx.bind_uniform_buffer(6, "core.view_properties"_external);
   ctx.bind_uniform_buffer(7, "shadow_map.matrices"_external);

   ctx.push_constant(ShadingPushConstants{
      Vector4(-30, 0, -5, 1.0),
      config.ambientOcclusion != AmbientOcclusionMethod::Disabled,
      config.shadowCasting == ShadowCastingMethod::RayTracing,
   });

   ctx.set_is_blending_enabled(false);
   ctx.set_depth_test_mode(graphics_api::DepthTestMode::Disabled);

   ctx.draw_full_screen_quad();

   this->render_particles(ctx);

   ctx.end_render_pass();

   blur_texture(ctx, "shading.bloom"_name, "shading.blurred_bloom"_name, GAPI_FORMAT(RGBA, Float16));
}

void ShadingStage::prepare_particles(render_core::BuildContext& ctx) const
{
   ctx.declare_buffer("particles"_name, sizeof(Particle) * g_particleCount);

   ctx.add_buffer_usage("particles"_name, graphics_api::BufferUsage::TransferDst);// Initially this buffer is filled from host

   ctx.bind_compute_shader("particles.cshader"_rc);

   ctx.bind_storage_buffer(0, "particles"_last_frame);
   ctx.bind_storage_buffer(1, "particles"_name);
   ctx.bind_uniform_buffer(2, "core.frame_params"_external);

   ctx.dispatch({1, 1, 1});
}

void ShadingStage::render_particles(render_core::BuildContext& ctx) const
{
   ctx.bind_vertex_shader("particles.vshader"_rc);

   ctx.bind_uniform_buffer(0, "core.view_properties"_external);
   ctx.bind_storage_buffer(1, "particles"_name);

   ctx.bind_fragment_shader("particles.fshader"_rc);

   ctx.bind_samplable_texture(2, "particle.tex"_rc);

   ctx.set_vertex_topology(graphics_api::VertexTopology::TriangleStrip);
   ctx.set_depth_test_mode(graphics_api::DepthTestMode::ReadOnly);

   ctx.draw_primitives(4, 0, 256, 0);
}

void ShadingStage::initialize_particles(render_core::JobGraph& ctx)
{
   Vector3 center{-30, 0, -4};
   Vector3 range{2, 2, 2};

   std::vector<Particle> particles;
   particles.resize(g_particleCount);

   std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
   std::default_random_engine generator{};

   for (auto& particle : particles) {
      auto offset = glm::vec3(dist(generator), dist(generator), dist(generator));
      particle.position = center + range * glm::normalize(offset);
      particle.velocity = 0.01f * glm::vec3(dist(generator), dist(generator), dist(generator));
      particle.animation = 0.5f * (1.0f + dist(generator));
      particle.rotation = 2.0f * g_pi * dist(generator);
      particle.angularVelocity = dist(generator);
      particle.scale = 0.5f + (1.0f + dist(generator));
   }

   GAPI_CHECK_STATUS(ctx.resources().buffer("particles"_name, 2).write_indirect(particles.data(), particles.size() * sizeof(Particle)));
}

}// namespace triglav::renderer::stage