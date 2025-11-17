#pragma once

#include "../UpdateViewParamsJob.hpp"
#include "IStage.hpp"

#include "triglav/render_core/RenderCore.hpp"

namespace triglav::renderer {
class Scene;
class BindlessScene;
}// namespace triglav::renderer

namespace triglav::renderer::stage {

class ShadowMapStage final : public IStage
{
 public:
   using Self = ShadowMapStage;

   ShadowMapStage(Scene& scene, BindlessScene& bindless_scene, UpdateViewParamsJob& update_view_params_job);

   void build_stage(render_core::BuildContext& ctx, const Config& config) const override;
   void render_cascade(render_core::BuildContext& ctx, Name pass_name, Name target_name, render_core::BufferRef view_props) const;

   void on_resource_definition(render_core::BuildContext& ctx) const;
   void on_view_properties_changed(render_core::BuildContext& ctx) const;
   void on_view_properties_not_changed(render_core::BuildContext& ctx) const;
   void on_finalize(render_core::BuildContext& ctx) const;
   void on_prepare_frame(render_core::JobGraph& graph, u32 frame_index) const;

 private:
   Scene& m_scene;
   BindlessScene& m_bindless_scene;

   TG_SINK(UpdateViewParamsJob, OnResourceDefinition);
   TG_SINK(UpdateViewParamsJob, OnViewPropertiesChanged);
   TG_SINK(UpdateViewParamsJob, OnViewPropertiesNotChanged);
   TG_SINK(UpdateViewParamsJob, OnFinalize);
   TG_SINK(UpdateViewParamsJob, OnPrepareFrame);
};

}// namespace triglav::renderer::stage
