#include "TerrainRenderer.hpp"

#include "triglav/engine/Engine.hpp"
#include "triglav/render_core/BuildContext.hpp"
#include "triglav/world/Terrain.hpp"

#include <memory>

namespace triglav::renderer {

using namespace name_literals;
using namespace render_core::literals;

engine::SystemRegisterer TERRAIN_RENDERER_REGISTERER{{
   .constructor = [](world::Level& /*level*/) -> std::unique_ptr<world::ISystem> { return std::make_unique<TerrainRenderer>(); },
   .components =
      std::vector<Name>{
         "triglav::world::TerrainComponent"_name,
      },
}};

namespace {

struct TerrainVertex
{
   Vector3 position;
   Vector2 uv;
};

graphics_api::Buffer generate_patch_vertices()
{
   static constexpr u32 w = 8;
   static constexpr u32 h = 8;
   static constexpr u32 vertices_count = 4 * w * h;
   static constexpr float size = 120.0;

   std::vector<TerrainVertex> vertices(vertices_count);


   for (u32 y = 0; y < h; ++y) {
      for (u32 x = 0; x < w; ++x) {
         const float uv_left = static_cast<float>(x) / static_cast<float>(w);
         const float uv_right = static_cast<float>(x + 1) / static_cast<float>(w);
         const float uv_bottom = static_cast<float>(y) / static_cast<float>(h);
         const float uv_top = static_cast<float>(y + 1) / static_cast<float>(h);

         const float pos_left = size * (2.0f * uv_left - 1.0f);
         const float pos_right = size * (2.0f * uv_right - 1.0f);
         const float pos_bottom = size * (2.0f * uv_bottom - 1.0f);
         const float pos_top = size * (2.0f * uv_top - 1.0f);

         const auto index = x + y * w;
         vertices[4 * index + 0] = {Vector3{pos_left, pos_bottom, 0.0f}, Vector2{uv_left, uv_bottom}};
         vertices[4 * index + 1] = {Vector3{pos_left, pos_top, 0.0f}, Vector2{uv_left, uv_top}};
         vertices[4 * index + 2] = {Vector3{pos_right, pos_top, 0.0f}, Vector2{uv_right, uv_top}};
         vertices[4 * index + 3] = {Vector3{pos_right, pos_bottom, 0.0f}, Vector2{uv_right, uv_bottom}};
      }
   }

   auto* device = engine::the().gfx_device();
   assert(device != nullptr);

   auto buffer = GAPI_CHECK(device->create_buffer(graphics_api::BufferUsage::TransferDst | graphics_api::BufferUsage::VertexBuffer,
                                                  vertices.size() * sizeof(TerrainVertex)));
   GAPI_CHECK_STATUS(buffer.write_indirect(vertices.data(), vertices.size() * sizeof(TerrainVertex)));
   return buffer;
}

graphics_api::Texture generate_terrain_texture(const graphics_api::ColorFormat format, const Vector2u dimensions, const u8* buffer_data)
{
   auto* device = engine::the().gfx_device();
   assert(device != nullptr);

   auto out_texture = GAPI_CHECK(device->create_texture(format, graphics_api::Resolution{dimensions.x, dimensions.y}));

   GAPI_CHECK_STATUS(out_texture.write(*device, buffer_data));

   out_texture.sampler_properties().address_u = graphics_api::TextureAddressMode::Clamp;
   out_texture.sampler_properties().address_v = graphics_api::TextureAddressMode::Clamp;
   out_texture.sampler_properties().address_w = graphics_api::TextureAddressMode::Clamp;
   return out_texture;
}

const render_core::VertexLayout terrain_layout = render_core::VertexLayout{sizeof(TerrainVertex)}
                                                    .add("position"_name, GAPI_FORMAT(RGB, Float32), offsetof(TerrainVertex, position))
                                                    .add("uv"_name, GAPI_FORMAT(RG, Float32), offsetof(TerrainVertex, uv));

}// namespace

TerrainRenderer::TerrainRenderer() :
    m_patch_vertices(generate_patch_vertices())
{
}

Name TerrainRenderer::system_name()
{
   return TAG;
}

void TerrainRenderer::on_level_loaded(world::Level& level)
{
   for (const auto& [entity_id, terrain_component] : level.all<world::TerrainComponent>()) {
      this->add_terrain_resources(entity_id, terrain_component.name);
   }
}

void TerrainRenderer::on_removed_entities(world::Level& /*level*/, const std::span<const world::EntityID> entity_ids)
{
   for (const world::EntityID entity_id : entity_ids) {
      m_terrain_resources.erase(entity_id);
   }
}

void TerrainRenderer::on_added_component(world::Level& level, Name /*component_name*/, world::ComponentID /*component_id*/,
                                         const std::span<const world::EntityID> entities)
{
   for (const world::EntityID entity_id : entities) {
      this->add_terrain_resources(entity_id, level.component<world::TerrainComponent>(entity_id).name);
   }
}

void TerrainRenderer::on_modified_component(world::Level& level, Name /*component_name*/, world::ComponentID /*component_id*/,
                                            std::span<const world::EntityID> entities)
{
   auto* device = engine::the().gfx_device();
   assert(device != nullptr);

   for (const auto entity_id : entities) {
      auto& resources = m_terrain_resources.at(entity_id);

      auto& terrain = engine::resource_manager().get(level.component<world::TerrainComponent>(entity_id).name);

      GAPI_CHECK_STATUS(resources.height_texture.write(*device, reinterpret_cast<const uint8_t*>(terrain.heightmap().data())));
      GAPI_CHECK_STATUS(resources.blending_texture.write(*device, reinterpret_cast<const uint8_t*>(terrain.blending().data())));
      GAPI_CHECK_STATUS(resources.materials_texture.write(*device, reinterpret_cast<const uint8_t*>(terrain.material_indices().data())));
   }
}

void TerrainRenderer::build_commands(render_core::BuildContext& ctx)
{
   for (const auto& [entity_id, resources] : m_terrain_resources) {
      ctx.bind_vertex_shader("shader/terrain/vertex.vshader"_rc);

      ctx.bind_hull_shader("shader/terrain/hull.hshader"_rc);

      ctx.bind_uniform_buffer(0, "core.view_properties"_external);

      ctx.bind_domain_shader("shader/terrain/domain.dshader"_rc);

      ctx.bind_uniform_buffer(1, "core.view_properties"_external);
      ctx.bind_samplable_texture(2, &resources.height_texture);
      ctx.bind_samplable_texture(3, &resources.blending_texture);

      ctx.bind_fragment_shader("shader/terrain/fragment.fshader"_rc);

      ctx.bind_samplable_texture(4, "engine/texture/grass.tex"_rc);
      ctx.bind_samplable_texture(5, "engine/texture/dirt.tex"_rc);

      ctx.set_vertex_topology(graphics_api::VertexTopology::PatchList);

      ctx.bind_vertex_layout(terrain_layout);
      ctx.bind_vertex_buffer(&m_patch_vertices);
      ctx.set_tesselation_control_points(4);

      ctx.draw_primitives(8 * 8 * 4, 0, 1, 0);
   }
}

void TerrainRenderer::add_terrain_resources(world::EntityID entity_id, const TerrainName terrain_name)
{
   auto& terrain = engine::resource_manager().get(terrain_name);

   m_terrain_resources.emplace(
      entity_id, TerrainResources{
                    .height_texture = generate_terrain_texture(GAPI_FORMAT(R, Float32), terrain.dimensions(),
                                                               reinterpret_cast<const u8*>(terrain.heightmap().data())),
                    .blending_texture = generate_terrain_texture(GAPI_FORMAT(RGBA, UNorm8), terrain.dimensions(),
                                                                 reinterpret_cast<const u8*>(terrain.blending().data())),
                    .materials_texture = generate_terrain_texture(GAPI_FORMAT(RGBA, UNorm8), terrain.dimensions(),
                                                                  reinterpret_cast<const u8*>(terrain.material_indices().data())),
                 });
}

}// namespace triglav::renderer
