#pragma once

#include "triglav/graphics_api/Buffer.hpp"
#include "triglav/graphics_api/Texture.hpp"
#include "triglav/world/Level.hpp"

#include <map>

namespace triglav::render_core {

class BuildContext;

}

namespace triglav::renderer {

struct TerrainResources
{
   graphics_api::Texture height_texture;
   graphics_api::Texture blending_texture;
   graphics_api::Texture materials_texture;
};

class TerrainRenderer : public world::ISystem
{
 public:
   TG_TAG_CLASS(triglav::renderer::TerrainRenderer)

   TerrainRenderer();

   Name system_name() override;
   void on_level_loaded(world::Level& level) override;
   void on_removed_entities(world::Level& level, std::span<const world::EntityID> ids) override;
   void on_added_component(world::Level& level, Name component_name, world::ComponentID component_id,
                           std::span<const world::EntityID> entities) override;
   void on_modified_component(world::Level& level, Name component_name, world::ComponentID component_id,
                              std::span<const world::EntityID> entities) override;

   void build_commands(render_core::BuildContext& ctx);

 private:
   void add_terrain_resources(world::EntityID entity_id, TerrainName terrain_name);

   graphics_api::Buffer m_patch_vertices;
   std::map<world::EntityID, TerrainResources> m_terrain_resources;
};

}// namespace triglav::renderer
