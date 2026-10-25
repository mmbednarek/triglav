#pragma once

#include "../UpdateViewParamsJob.hpp"
#include "IStage.hpp"

#include "triglav/render_core/RenderCore.hpp"
#include "triglav/render_objects/Material.hpp"

namespace triglav::renderer {
class ShadowMapManager;
class BindlessScene;
class TerrainRenderer;
class ViewContext;
}// namespace triglav::renderer

namespace triglav::renderer::stage {

class ShadowMapStage final : public IStage
{
 public:
   using Self = ShadowMapStage;

   ShadowMapStage(ShadowMapManager& shadow_map_manager, BindlessScene& bindless_scene, UpdateViewParamsJob& update_view_params_job,
                  TerrainRenderer& terrain_renderer);

   void build_stage(render_core::BuildContext& ctx, const Config& config) const override;
   void render_cascade(render_core::BuildContext& ctx, Name pass_name, Name target_name, u32 matrix_offset) const;
   void render_geometry(render_core::BuildContext& ctx, render_core::BufferRef matrix_buffer, u32 matrix_offset,
                        const render_objects::MaterialVertexLayoutInfo& layout_info) const;

   void on_resource_definition(render_core::BuildContext& ctx) const;
   void on_view_properties_changed(render_core::BuildContext& ctx) const;
   void on_view_properties_not_changed(render_core::BuildContext& ctx) const;
   void on_finalize(render_core::BuildContext& ctx) const;
   void on_prepare_frame(render_core::JobGraph& graph, u32 frame_index) const;

 private:
   ShadowMapManager& m_shadow_map_manager;
   BindlessScene& m_bindless_scene;
   TerrainRenderer& m_terrain_renderer;
   u32 m_matrix_buffer_stride;

   TG_SINK(OnResourceDefinition);
   TG_SINK(OnViewPropertiesChanged);
   TG_SINK(OnViewPropertiesNotChanged);
   TG_SINK(OnFinalize);
   TG_SINK(OnPrepareFrame);
};

}// namespace triglav::renderer::stage
