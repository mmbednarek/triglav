#include "stage/RayTracingStage.hpp"

#include "RayTracingScene.hpp"
#include "Util.hpp"

#include "triglav/render_core/BuildContext.hpp"

#include <Config.hpp>


namespace triglav::renderer::stage {

using namespace name_literals;
using namespace render_core::literals;

namespace gapi = graphics_api;

RayTracingStage::RayTracingStage(RayTracingScene& rt_scene) :
    m_rt_scene(rt_scene)
{
}

void RayTracingStage::build_stage(render_core::BuildContext& ctx, const Config& config) const
{
   ctx.declare_screen_size_texture("ray_tracing.shadows"_name, GAPI_FORMAT(R, UNorm8));
   if (!config.is_any_rt_feature_enabled()) {
      return;
   }

   ctx.declare_screen_size_texture("ray_tracing.ambient_occlusion"_name, GAPI_FORMAT(R, UNorm8));

   ctx.set_bind_stages(gapi::PipelineStage::RayGenerationShader | gapi::PipelineStage::ClosestHitShader);

   ctx.bind_acceleration_structure(0, *m_rt_scene.m_tl_acceleration_structure);

   ctx.bind_rt_generation_shader("shader/ray_tracing/general.rgenshader"_rc);

   ctx.bind_rw_texture(1, "ray_tracing.ambient_occlusion"_name);
   ctx.bind_rw_texture(2, "ray_tracing.shadows"_name);
   ctx.bind_uniform_buffer(3, "core.view_properties"_external);

   ctx.bind_rt_miss_shader("shader/ray_tracing/general.rmissshader"_rc);
   ctx.bind_rt_miss_shader("shader/ray_tracing/shadow.rmissshader"_rc);
   ctx.bind_rt_miss_shader("shader/ray_tracing/ambient_occlusion.rmissshader"_rc);
   ctx.bind_rt_closest_hit_shader("shader/ray_tracing/general.rchitshader"_rc);

   ctx.bind_storage_buffer(4, &m_rt_scene.m_object_buffer);
   auto light_dir{m_rt_scene.m_scene.shadow_map_camera(0).orientation() * glm::vec3(0.0f, 1.0f, 0.0f)};
   ctx.push_constant(light_dir);

   ctx.bind_rt_closest_hit_shader("shader/ray_tracing/shadow.rchitshader"_rc);
   ctx.bind_rt_closest_hit_shader("shader/ray_tracing/ambient_occlusion.rchitshader"_rc);

   render_core::RayTracingShaderGroup ray_gen_group{};
   ray_gen_group.type = render_core::RayTracingShaderGroupType::General;
   ray_gen_group.general_shader = "shader/ray_tracing/general.rgenshader"_rc;
   ctx.bind_rt_shader_group(ray_gen_group);

   render_core::RayTracingShaderGroup ray_miss_group{};
   ray_miss_group.type = render_core::RayTracingShaderGroupType::General;
   ray_miss_group.general_shader = "shader/ray_tracing/general.rmissshader"_rc;
   ctx.bind_rt_shader_group(ray_miss_group);

   render_core::RayTracingShaderGroup ray_shadow_miss_group{};
   ray_shadow_miss_group.type = render_core::RayTracingShaderGroupType::General;
   ray_shadow_miss_group.general_shader = "shader/ray_tracing/shadow.rmissshader"_rc;
   ctx.bind_rt_shader_group(ray_shadow_miss_group);

   render_core::RayTracingShaderGroup ray_ao_miss_group{};
   ray_ao_miss_group.type = render_core::RayTracingShaderGroupType::General;
   ray_ao_miss_group.general_shader = "shader/ray_tracing/ambient_occlusion.rmissshader"_rc;
   ctx.bind_rt_shader_group(ray_ao_miss_group);

   render_core::RayTracingShaderGroup ray_hit_group{};
   ray_hit_group.type = render_core::RayTracingShaderGroupType::Triangles;
   ray_hit_group.closest_hit_shader = "shader/ray_tracing/general.rchitshader"_rc;
   ctx.bind_rt_shader_group(ray_hit_group);

   render_core::RayTracingShaderGroup ray_shadow_hit_group{};
   ray_shadow_hit_group.type = render_core::RayTracingShaderGroupType::Triangles;
   ray_shadow_hit_group.closest_hit_shader = "shader/ray_tracing/shadow.rchitshader"_rc;
   ctx.bind_rt_shader_group(ray_shadow_hit_group);

   render_core::RayTracingShaderGroup ray_ao_hit_group{};
   ray_ao_hit_group.type = render_core::RayTracingShaderGroupType::Triangles;
   ray_ao_hit_group.closest_hit_shader = "shader/ray_tracing/ambient_occlusion.rchitshader"_rc;
   ctx.bind_rt_shader_group(ray_ao_hit_group);

   ctx.set_rt_max_recursion_depth(2);

   ctx.trace_rays({ctx.screen_size().x, ctx.screen_size().y, 1});

   blur_texture(ctx, "ray_tracing.ambient_occlusion"_name, "ray_tracing.ambient_occlusion.blurred"_name, GAPI_FORMAT(R, UNorm8));
}

}// namespace triglav::renderer::stage