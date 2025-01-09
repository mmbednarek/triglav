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
   alignas(16) Matrix4x4 orientation;
   float nearPlane;
   float farPlane;
};

class UpdateViewParamsJob
{
 public:
   using Self = UpdateViewParamsJob;

   TG_EVENT(OnResourceDefinition, render_core::BuildContext&)
   TG_EVENT(OnViewPropertiesChanged, render_core::BuildContext&)
   TG_EVENT(OnViewPropertiesNotChanged, render_core::BuildContext&)
   TG_EVENT(OnFinalize, render_core::BuildContext&)

   static constexpr auto JobName = make_name_id("job.update_view_properties");

   explicit UpdateViewParamsJob(Scene& scene);

   void build_job(render_core::BuildContext& ctx) const;
   void prepare_frame(render_core::JobGraph& graph, u32 frameIndex);
   void on_updated(const Camera& camera);

 private:
   bool m_updatedViewProperties = true;
   ViewProperties m_viewProperties{};

   TG_SINK(Scene, OnViewUpdated);
};

}// namespace triglav::renderer