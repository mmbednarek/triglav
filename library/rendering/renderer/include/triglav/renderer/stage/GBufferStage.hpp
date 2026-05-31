#pragma once

#include "../Scene.hpp"
#include "IStage.hpp"

#include "triglav/event/Delegate.hpp"
#include "triglav/render_core/RenderCore.hpp"
#include "triglav/render_objects/Mesh.hpp"

namespace triglav::graphics_api {
class Device;
}

namespace triglav::renderer {
class BindlessScene;
}

namespace triglav::renderer::stage {

class GBufferStage final : public IStage
{
 public:
   using Self = GBufferStage;

   GBufferStage(graphics_api::Device& device, BindlessScene& bindless_scene);

   void build_stage(render_core::BuildContext& ctx, const Config& config) const override;

   void build_skybox(render_core::BuildContext& ctx) const;
   void build_ground(render_core::BuildContext& ctx) const;
   void build_geometry(render_core::BuildContext& ctx) const;
   void build_terrain(render_core::BuildContext& ctx) const;

   void on_terrain_updated(Vector2i size, const std::vector<float>& height, const std::vector<u8>& blending) const;

 private:
   void draw_objects_with_render_info(render_core::BuildContext& ctx, const render_objects::MaterialGeometryRenderInfo& info) const;

   graphics_api::Device& m_device;
   geometry::DeviceMesh m_mesh;
   graphics_api::Texture m_terrain_texture;
   graphics_api::Texture m_terrain_blend_texture;
   graphics_api::Buffer m_terrain_vertices;
   BindlessScene& m_bindless_scene;

   TG_SINK(Scene, OnTerrainUpdated);
};

}// namespace triglav::renderer::stage
