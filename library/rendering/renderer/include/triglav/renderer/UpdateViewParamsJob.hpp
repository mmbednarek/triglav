#pragma once

#include "Camera.hpp"
#include "Scene.hpp"

#include "triglav/Int.hpp"
#include "triglav/Math.hpp"
#include "triglav/Name.hpp"
#include "triglav/event/Delegate.hpp"

namespace triglav::render_core {
class BuildContext;
class JobGraph;
}// namespace triglav::render_core

namespace triglav::renderer {

struct ViewProperties
{
   alignas(16) Matrix4x4 view;
   alignas(16) Matrix4x4 projection;
   alignas(16) Matrix4x4 inverted_view;
   alignas(16) Matrix4x4 inverted_projection;
   alignas(16) Matrix4x4 orientation;
   float near_plane;
   float far_plane;
};

class UpdateViewParamsJob
{
 public:
   using Self = UpdateViewParamsJob;

   TG_EVENT(OnResourceDefinition, render_core::BuildContext&)
   TG_EVENT(OnViewPropertiesChanged, render_core::BuildContext&)
   TG_EVENT(OnViewPropertiesNotChanged, render_core::BuildContext&)
   TG_EVENT(OnFinalize, render_core::BuildContext&)
   TG_EVENT(OnPrepareFrame, render_core::JobGraph&, u32)

   static constexpr auto JobName = make_name_id("job.update_view_properties");

   explicit UpdateViewParamsJob(Scene& scene);

   void build_job(render_core::BuildContext& ctx) const;
   void prepare_frame(render_core::JobGraph& graph, u32 frame_index, float delta_time);
   void on_updated(const Camera& camera);

 private:
   bool m_updated_view_properties = true;
   ViewProperties m_view_properties{};

   TG_SINK(Scene, OnViewUpdated);
};

}// namespace triglav::renderer