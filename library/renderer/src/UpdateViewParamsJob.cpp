#include "UpdateViewParamsJob.hpp"

#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/JobGraph.hpp"

namespace triglav::renderer {

using namespace name_literals;
using namespace render_core::literals;
namespace gapi = graphics_api;

struct FrameParameters
{
   float deltaTime;
   u32 randomSeed;
};

UpdateViewParamsJob::UpdateViewParamsJob(Scene& scene) :
    TG_CONNECT(scene, OnViewUpdated, on_updated)
{
}

void UpdateViewParamsJob::build_job(render_core::BuildContext& ctx) const
{
   ctx.declare_buffer("core.view_properties"_name, sizeof(ViewProperties));
   ctx.declare_staging_buffer("core.view_properties.staging"_name, sizeof(ViewProperties));
   ctx.declare_buffer("core.frame_params"_name, sizeof(FrameParameters));
   ctx.declare_staging_buffer("core.frame_params.staging"_name, sizeof(FrameParameters));

   event_OnResourceDefinition.publish(ctx);

   ctx.declare_flag("have_view_properties_changed"_name);

   ctx.copy_buffer("core.frame_params.staging"_name, "core.frame_params"_name);

   ctx.if_enabled("have_view_properties_changed"_name);
   {
      ctx.copy_buffer("core.view_properties.staging"_name, "core.view_properties"_name);
      event_OnViewPropertiesChanged.publish(ctx);
      ctx.end_if();
   }
   ctx.if_disabled("have_view_properties_changed"_name);
   {
      // if not changed, copy from last frame
      ctx.copy_buffer("core.view_properties"_last_frame, "core.view_properties"_name);
      event_OnViewPropertiesNotChanged.publish(ctx);
      ctx.end_if();
   }

   ctx.export_buffer("core.view_properties"_name, gapi::PipelineStage::VertexShader, gapi::BufferAccess::UniformRead,
                     gapi::BufferUsage::UniformBuffer);
   ctx.export_buffer("core.frame_params"_name, gapi::PipelineStage::ComputeShader, gapi::BufferAccess::UniformRead,
                     gapi::BufferUsage::UniformBuffer);

   event_OnFinalize.publish(ctx);
}

void UpdateViewParamsJob::prepare_frame(render_core::JobGraph& graph, const u32 frameIndex, const float deltaTime)
{
   if (!m_updatedViewProperties) {
      graph.disable_flag(JobName, "have_view_properties_changed"_name);
      return;
   }

   graph.enable_flag(JobName, "have_view_properties_changed"_name);

   const auto viewPropertiesMem = GAPI_CHECK(graph.resources().buffer("core.view_properties.staging"_name, frameIndex).map_memory());
   viewPropertiesMem.write(&m_viewProperties, sizeof(ViewProperties));

   FrameParameters frameParams{};
   frameParams.deltaTime = deltaTime;
   frameParams.randomSeed = rand();
   const auto frameParamsMem = GAPI_CHECK(graph.resources().buffer("core.frame_params.staging"_name, frameIndex).map_memory());
   frameParamsMem.write(&frameParams, sizeof(FrameParameters));

   event_OnPrepareFrame.publish(graph, frameIndex);

   m_updatedViewProperties = false;
}

void UpdateViewParamsJob::on_updated(const Camera& camera)
{
   m_updatedViewProperties = true;
   m_viewProperties.projection = camera.projection_matrix();
   m_viewProperties.view = camera.view_matrix();
   m_viewProperties.invertedView = glm::inverse(camera.view_matrix());
   m_viewProperties.invertedProjection = glm::inverse(camera.projection_matrix());
   m_viewProperties.orientation =
      glm::inverse(glm::rotate(glm::mat4(camera.orientation()), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
   m_viewProperties.farPlane = camera.far_plane();
   m_viewProperties.nearPlane = camera.near_plane();
}

}// namespace triglav::renderer