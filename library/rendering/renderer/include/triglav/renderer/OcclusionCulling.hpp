#pragma once

#include "triglav/render_core/RenderCore.hpp"

#include "UpdateViewParamsJob.hpp"

namespace triglav::graphics_api {
class Device;
}

namespace triglav::render_core {
class JobGraph;
}

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
   static void reset_buffers(graphics_api::Device& device, render_core::JobGraph& graph);

 private:
   void draw_pre_pass_objects(render_core::BuildContext& ctx, render_core::BufferRef objectBuffer, render_core::BufferRef countBuffer,
                              u32 index) const;

   BindlessScene& m_bindlessScene;

   TG_SINK(UpdateViewParamsJob, OnResourceDefinition);
   TG_SINK(UpdateViewParamsJob, OnViewPropertiesChanged);
   TG_SINK(UpdateViewParamsJob, OnViewPropertiesNotChanged);
   TG_SINK(UpdateViewParamsJob, OnFinalize);
};

}// namespace triglav::renderer
