#include "UpdateViewParamsJob.hpp"

#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/JobGraph.hpp"

namespace triglav::renderer {

using namespace name_literals;
using namespace render_core::literals;
namespace gapi = graphics_api;

struct FrameParameters
{
   float delta_time;
   u32 random_seed;
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

void UpdateViewParamsJob::prepare_frame(render_core::JobGraph& graph, const u32 frame_index, const float delta_time)
{
   if (!m_updated_view_properties) {
      graph.disable_flag(JobName, "have_view_properties_changed"_name);
      return;
   }

   graph.enable_flag(JobName, "have_view_properties_changed"_name);

   const auto view_properties_mem = GAPI_CHECK(graph.resources().buffer("core.view_properties.staging"_name, frame_index).map_memory());
   view_properties_mem.write(&m_view_properties, sizeof(ViewProperties));

   FrameParameters frame_params{};
   frame_params.delta_time = delta_time;
   frame_params.random_seed = rand();
   const auto frame_params_mem = GAPI_CHECK(graph.resources().buffer("core.frame_params.staging"_name, frame_index).map_memory());
   frame_params_mem.write(&frame_params, sizeof(FrameParameters));

   event_OnPrepareFrame.publish(graph, frame_index);

   m_updated_view_properties = false;
}

void UpdateViewParamsJob::on_updated(const Camera& camera)
{
   m_updated_view_properties = true;
   m_view_properties.projection = camera.projection_matrix();
   m_view_properties.view = camera.view_matrix();
   m_view_properties.inverted_view = glm::inverse(camera.view_matrix());
   m_view_properties.inverted_projection = glm::inverse(camera.projection_matrix());
   m_view_properties.orientation =
      glm::inverse(glm::rotate(glm::mat4(camera.orientation()), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
   m_view_properties.far_plane = camera.far_plane();
   m_view_properties.near_plane = camera.near_plane();
}

}// namespace triglav::renderer