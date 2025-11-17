#include "ui/SpriteRenderer.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/JobGraph.hpp"
#include "triglav/resource/ResourceManager.hpp"

namespace triglav::renderer::ui {

using namespace name_literals;
using namespace render_core::literals;

using graphics_api::BufferUsage;
using ui_core::SpriteId;

constexpr auto g_insertion_buffer_size = 128;
constexpr auto g_removal_buffer_size = 128;
constexpr auto g_draw_call_buffer_size = 512;

constexpr auto g_cs_group_size = 256;

namespace {

struct DrawCall
{
   u32 vertex_count;
   u32 instance_count;
   u32 first_vertex;
   u32 first_instance;

   SpritePrimitive primitive;
};

static_assert(sizeof(DrawCall) % 16 == 0);

SpritePrimitive to_primitive(const ui_core::Sprite& sprite, const u32 image_id, const Vector2 texture_size)
{
   Vector4 uv_rect{0, 0, 1, 1};

   if (sprite.texture_region.has_value()) {
      uv_rect = Vector4(sprite.texture_region->x, sprite.texture_region->y, sprite.texture_region->z, sprite.texture_region->w) /
                Vector4(texture_size, texture_size);
   }

   SpritePrimitive out;
   out.sprite_rect = {sprite.position, sprite.size};
   out.uv_rect = uv_rect;
   out.cropping_mask = sprite.crop;
   out.image_id = image_id;
   return out;
}

}// namespace

SpriteRenderer::SpriteRenderer(ui_core::Viewport& viewport, resource::ResourceManager& resource_manager) :
    m_resource_manager(resource_manager),
    TG_CONNECT(viewport, OnAddedSprite, on_added_sprite),
    TG_CONNECT(viewport, OnUpdatedSprite, on_updated_sprite),
    TG_CONNECT(viewport, OnRemovedSprite, on_removed_sprite)
{
}

void SpriteRenderer::on_added_sprite(const SpriteId id, const ui_core::Sprite& sprite)
{
   this->on_updated_sprite(id, sprite);
}

void SpriteRenderer::on_updated_sprite(const SpriteId id, const ui_core::Sprite& sprite)
{
   const auto tex_size_res = m_resource_manager.get(sprite.texture).resolution();
   const Vector2 tex_size{tex_size_res.width, tex_size_res.height};

   for (auto& updates : m_frame_updates) {
      updates.add_or_update(id, to_primitive(sprite, this->get_texture_id(sprite.texture), tex_size));
   }
}

void SpriteRenderer::on_removed_sprite(const SpriteId id)
{
   for (auto& updates : m_frame_updates) {
      updates.remove(id);
   }
}

void SpriteRenderer::set_object(const u32 index, const SpritePrimitive& prim)
{
   assert(index <= g_draw_call_buffer_size);
   assert(m_staging_insertions_top < g_insertion_buffer_size);
   m_staging_insertions[m_staging_insertions_top].dst_index = index;
   m_staging_insertions[m_staging_insertions_top].primitive = prim;
   m_staging_insertions_top++;
}

void SpriteRenderer::move_object(const u32 src, const u32 dst)
{
   assert(src <= g_draw_call_buffer_size);
   assert(dst <= g_draw_call_buffer_size);
   assert(m_staging_removals_top < g_removal_buffer_size);
   m_staging_removals[m_staging_removals_top].src_id = src;
   m_staging_removals[m_staging_removals_top].dst_id = dst;
   m_staging_removals_top++;
}

void SpriteRenderer::prepare_frame(render_core::JobGraph& graph, const u32 frame_index)
{
   // Insertions
   const auto insertions = GAPI_CHECK(graph.resources().buffer("user_interface.sprite.insertion.staging"_name, frame_index).map_memory());
   m_staging_insertions = &insertions.cast<SpriteWriteData>();
   m_staging_insertions_top = 0;

   // Removals
   const auto removals = GAPI_CHECK(graph.resources().buffer("user_interface.sprite.removal.staging"_name, frame_index).map_memory());
   m_staging_removals = &removals.cast<SpriteCopyInfo>();
   m_staging_removals_top = 0;

   m_frame_updates[frame_index].write_to_buffers(*this);

   // Write calls
   const auto insertion_dims =
      GAPI_CHECK(graph.resources().buffer("user_interface.sprite.insertion.indirect_buffer"_name, frame_index).map_memory());
   insertion_dims.cast<Vector3u>() = {divide_rounded_up(m_staging_insertions_top, g_cs_group_size), 1, 1};
   const auto insertion_count =
      GAPI_CHECK(graph.resources().buffer("user_interface.sprite.insertion.count"_name, frame_index).map_memory());
   insertion_count.cast<u32>() = m_staging_insertions_top;

   const auto removal_dims =
      GAPI_CHECK(graph.resources().buffer("user_interface.sprite.removal.indirect_buffer"_name, frame_index).map_memory());
   removal_dims.cast<Vector3u>() = {divide_rounded_up(m_staging_removals_top, g_cs_group_size), 1, 1};
   const auto removal_count = GAPI_CHECK(graph.resources().buffer("user_interface.sprite.removal.count"_name, frame_index).map_memory());
   removal_count.cast<u32>() = m_staging_removals_top;

   // Fill count
   const auto count = GAPI_CHECK(graph.resources().buffer("user_interface.sprite.count"_name, frame_index).map_memory());
   count.cast<u32>() = m_frame_updates[frame_index].top_index();
}

void SpriteRenderer::build_data_update(render_core::BuildContext& ctx) const
{
   ctx.declare_buffer("user_interface.sprite.insertion"_name, sizeof(SpriteWriteData) * g_insertion_buffer_size);
   ctx.declare_staging_buffer("user_interface.sprite.insertion.staging"_name, sizeof(SpriteWriteData) * g_insertion_buffer_size);
   ctx.declare_staging_buffer("user_interface.sprite.insertion.count"_name, sizeof(u32));

   ctx.declare_buffer("user_interface.sprite.removal"_name, sizeof(SpriteCopyInfo) * g_removal_buffer_size);
   ctx.declare_staging_buffer("user_interface.sprite.removal.staging"_name, sizeof(SpriteCopyInfo) * g_removal_buffer_size);
   ctx.declare_staging_buffer("user_interface.sprite.removal.count"_name, sizeof(u32));

   ctx.declare_buffer("user_interface.sprite.draw_calls"_name, sizeof(DrawCall) * g_draw_call_buffer_size);

   ctx.declare_staging_buffer("user_interface.sprite.insertion.indirect_buffer"_name, sizeof(Vector3u));
   ctx.declare_staging_buffer("user_interface.sprite.removal.indirect_buffer"_name, sizeof(Vector3u));

   ctx.declare_staging_buffer("user_interface.sprite.count"_name, sizeof(u32));

   // Copy buffers
   ctx.copy_buffer("user_interface.sprite.insertion.staging"_name, "user_interface.sprite.insertion"_name);
   ctx.copy_buffer("user_interface.sprite.removal.staging"_name, "user_interface.sprite.removal"_name);

   // Execute insertion
   ctx.bind_compute_shader("sprite/insertion.cshader"_rc);
   ctx.bind_uniform_buffer(0, "user_interface.sprite.insertion.count"_name);
   ctx.bind_storage_buffer(1, "user_interface.sprite.insertion"_name);
   ctx.bind_storage_buffer(2, "user_interface.sprite.draw_calls"_name);
   ctx.dispatch_indirect("user_interface.sprite.insertion.indirect_buffer"_name);

   // Execute removal
   ctx.bind_compute_shader("sprite/removal.cshader"_rc);
   ctx.bind_uniform_buffer(0, "user_interface.sprite.removal.count"_name);
   ctx.bind_storage_buffer(1, "user_interface.sprite.removal"_name);
   ctx.bind_storage_buffer(2, "user_interface.sprite.draw_calls"_name);
   ctx.dispatch_indirect("user_interface.sprite.removal.indirect_buffer"_name);

   ctx.export_buffer("user_interface.sprite.draw_calls"_name, graphics_api::PipelineStage::VertexShader,
                     graphics_api::BufferAccess::ShaderRead,
                     graphics_api::BufferUsage::StorageBuffer | graphics_api::BufferUsage::Indirect);
   ctx.export_buffer("user_interface.sprite.count"_name, graphics_api::PipelineStage::DrawIndirect,
                     graphics_api::BufferAccess::IndirectCmdRead, graphics_api::BufferUsage::Indirect);
}

void SpriteRenderer::build_render_ui(render_core::BuildContext& ctx)
{
   if (m_textures.empty())
      return;

   ctx.bind_vertex_shader("sprite/render.vshader"_rc);
   ctx.bind_uniform_buffer(0, "ui.viewport_info"_external);
   ctx.bind_storage_buffer(1, "user_interface.sprite.draw_calls"_external);

   ctx.bind_fragment_shader("sprite/render.fshader"_rc);

   ctx.bind_sampled_texture_array(2, m_textures);

   ctx.draw_indirect_with_count("user_interface.sprite.draw_calls"_external, "user_interface.sprite.count"_external,
                                g_draw_call_buffer_size, sizeof(DrawCall));
}

u32 SpriteRenderer::get_texture_id(const TextureName name)
{
   const auto it = m_texture_ids.find(name);
   if (it != m_texture_ids.end()) {
      return it->second;
   }

   const auto id = static_cast<u32>(m_textures.size());
   m_textures.emplace_back(name);
   m_texture_ids.emplace(name, id);
   return id;
}

}// namespace triglav::renderer::ui
