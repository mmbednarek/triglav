#include "stage/RayTracingStage.hpp"

#include "RayTracingScene.hpp"
#include "Util.hpp"

#include "triglav/render_core/BuildContext.hpp"

#include <Config.hpp>


namespace triglav::renderer::stage {

using namespace name_literals;
using namespace render_core::literals;

namespace gapi = graphics_api;

RayTracingStage::RayTracingStage(RayTracingScene& rtScene) :
    m_rtScene(rtScene)
{
}

void RayTracingStage::build_stage(render_core::BuildContext& ctx, const Config& config) const
{
   ctx.declare_screen_size_texture("ray_tracing.ambient_occlusion"_name, GAPI_FORMAT(R, UNorm8));
   ctx.declare_screen_size_texture("ray_tracing.shadows"_name, GAPI_FORMAT(R, UNorm8));

   if (!config.is_any_rt_feature_enabled()) {
      return;
   }

   ctx.set_bind_stages(gapi::PipelineStage::RayGenerationShader | gapi::PipelineStage::ClosestHitShader);

   ctx.bind_acceleration_structure(0, *m_rtScene.m_tlAccelerationStructure);

   ctx.bind_rt_generation_shader("rt_general.rgenshader"_rc);

   ctx.bind_rw_texture(1, "ray_tracing.ambient_occlusion"_name);
   ctx.bind_rw_texture(2, "ray_tracing.shadows"_name);
   ctx.bind_uniform_buffer(3, "core.view_properties"_external);

   ctx.bind_rt_miss_shader("rt_general.rmissshader"_rc);
   ctx.bind_rt_miss_shader("rt_shadow.rmissshader"_rc);
   ctx.bind_rt_miss_shader("rt_ao.rmissshader"_rc);
   ctx.bind_rt_closest_hit_shader("rt_general.rchitshader"_rc);

   ctx.bind_storage_buffer(4, &m_rtScene.m_objectBuffer);
   auto lightDir{m_rtScene.m_scene.shadow_map_camera(0).orientation() * glm::vec3(0.0f, 1.0f, 0.0f)};
   ctx.push_constant(lightDir);

   ctx.bind_rt_closest_hit_shader("rt_shadow.rchitshader"_rc);
   ctx.bind_rt_closest_hit_shader("rt_ao.rchitshader"_rc);

   render_core::RayTracingShaderGroup rayGenGroup{};
   rayGenGroup.type = render_core::RayTracingShaderGroupType::General;
   rayGenGroup.generalShader = "rt_general.rgenshader"_rc;
   ctx.bind_rt_shader_group(rayGenGroup);

   render_core::RayTracingShaderGroup rayMissGroup{};
   rayMissGroup.type = render_core::RayTracingShaderGroupType::General;
   rayMissGroup.generalShader = "rt_general.rmissshader"_rc;
   ctx.bind_rt_shader_group(rayMissGroup);

   render_core::RayTracingShaderGroup rayShadowMissGroup{};
   rayShadowMissGroup.type = render_core::RayTracingShaderGroupType::General;
   rayShadowMissGroup.generalShader = "rt_shadow.rmissshader"_rc;
   ctx.bind_rt_shader_group(rayShadowMissGroup);

   render_core::RayTracingShaderGroup rayAoMissGroup{};
   rayAoMissGroup.type = render_core::RayTracingShaderGroupType::General;
   rayAoMissGroup.generalShader = "rt_ao.rmissshader"_rc;
   ctx.bind_rt_shader_group(rayAoMissGroup);

   render_core::RayTracingShaderGroup rayHitGroup{};
   rayHitGroup.type = render_core::RayTracingShaderGroupType::Triangles;
   rayHitGroup.closestHitShader = "rt_general.rchitshader"_rc;
   ctx.bind_rt_shader_group(rayHitGroup);

   render_core::RayTracingShaderGroup rayShadowHitGroup{};
   rayShadowHitGroup.type = render_core::RayTracingShaderGroupType::Triangles;
   rayShadowHitGroup.closestHitShader = "rt_shadow.rchitshader"_rc;
   ctx.bind_rt_shader_group(rayShadowHitGroup);

   render_core::RayTracingShaderGroup rayAoHitGroup{};
   rayAoHitGroup.type = render_core::RayTracingShaderGroupType::Triangles;
   rayAoHitGroup.closestHitShader = "rt_ao.rchitshader"_rc;
   ctx.bind_rt_shader_group(rayAoHitGroup);

   ctx.set_rt_max_recursion_depth(2);

   ctx.trace_rays({ctx.screen_size().x, ctx.screen_size().y, 1});

   blur_texture(ctx, "ray_tracing.ambient_occlusion"_name, "ray_tracing.ambient_occlusion.blurred"_name, GAPI_FORMAT(R, UNorm8));
}

}// namespace triglav::renderer::stage