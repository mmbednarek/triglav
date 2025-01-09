#pragma once

#include "UpdateViewParamsJob.hpp"

namespace triglav::renderer {

class BindlessScene;

class OcclusionCulling
{
 public:
   using Self = OcclusionCulling;

   explicit OcclusionCulling(UpdateViewParamsJob& updateViewJob, BindlessScene& bindlessScene);

   void on_resource_definition(render_core::BuildContext& ctx) const;
   void on_view_properties_changed(render_core::BuildContext& ctx) const;
   void on_view_properties_not_changed(render_core::BuildContext& ctx) const;
   void on_finalize(render_core::BuildContext& ctx) const;

 private:
   BindlessScene& m_bindlessScene;

   TG_SINK(UpdateViewParamsJob, OnResourceDefinition);
   TG_SINK(UpdateViewParamsJob, OnViewPropertiesChanged);
   TG_SINK(UpdateViewParamsJob, OnViewPropertiesNotChanged);
   TG_SINK(UpdateViewParamsJob, OnFinalize);
};

}// namespace triglav::renderer
