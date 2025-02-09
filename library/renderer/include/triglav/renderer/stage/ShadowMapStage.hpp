#pragma once

#include "../UpdateViewParamsJob.hpp"
#include "IStage.hpp"

#include <BindlessScene.hpp>

namespace triglav::renderer {
class Scene;
class BindlessScene;
}// namespace triglav::renderer

namespace triglav::renderer::stage {

class ShadowMapStage final : public IStage
{
 public:
   using Self = ShadowMapStage;

   ShadowMapStage(Scene& scene, BindlessScene& bindlessScene, UpdateViewParamsJob& updateViewParamsJob);

   void build_stage(render_core::BuildContext& ctx, const Config& config) const override;
   void render_cascade(render_core::BuildContext& ctx, Name passName, Name targetName, render_core::BufferRef viewProps) const;

   void on_resource_definition(render_core::BuildContext& ctx) const;
   void on_view_properties_changed(render_core::BuildContext& ctx) const;
   void on_view_properties_not_changed(render_core::BuildContext& ctx) const;
   void on_finalize(render_core::BuildContext& ctx) const;
   void on_prepare_frame(render_core::JobGraph& graph, u32 frameIndex) const;

 private:
   Scene& m_scene;
   BindlessScene& m_bindlessScene;

   TG_SINK(UpdateViewParamsJob, OnResourceDefinition);
   TG_SINK(UpdateViewParamsJob, OnViewPropertiesChanged);
   TG_SINK(UpdateViewParamsJob, OnViewPropertiesNotChanged);
   TG_SINK(UpdateViewParamsJob, OnFinalize);
   TG_SINK(UpdateViewParamsJob, OnPrepareFrame);
};

}// namespace triglav::renderer::stage
