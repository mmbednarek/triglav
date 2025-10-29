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

constexpr auto g_insertionBufferSize = 128;
constexpr auto g_removalBufferSize = 128;
constexpr auto g_drawCallBufferSize = 512;

constexpr auto g_csGroupSize = 256;

namespace {

struct DrawCall
{
   u32 vertexCount;
   u32 instanceCount;
   u32 firstVertex;
   u32 firstInstance;

   SpritePrimitive primitive;
};

static_assert(sizeof(DrawCall) % 16 == 0);

SpritePrimitive to_primitive(const ui_core::Sprite& sprite, const u32 imageID, const Vector2 textureSize)
{
   Vector4 uvRect{0, 0, 1, 1};

   if (sprite.textureRegion.has_value()) {
      uvRect = Vector4(sprite.textureRegion->x, sprite.textureRegion->y, sprite.textureRegion->z, sprite.textureRegion->w) /
               Vector4(textureSize, textureSize);
   }

   SpritePrimitive out;
   out.spriteRect = {sprite.position, sprite.size};
   out.uvRect = uvRect;
   out.croppingMask = sprite.crop;
   out.imageID = imageID;
   return out;
}

}// namespace

SpriteRenderer::SpriteRenderer(ui_core::Viewport& viewport, resource::ResourceManager& resourceManager) :
    m_resourceManager(resourceManager),
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
   const auto texSizeRes = m_resourceManager.get(sprite.texture).resolution();
   const Vector2 texSize{texSizeRes.width, texSizeRes.height};

   for (auto& updates : m_frameUpdates) {
      updates.add_or_update(id, to_primitive(sprite, this->get_texture_id(sprite.texture), texSize));
   }
}

void SpriteRenderer::on_removed_sprite(const SpriteId id)
{
   for (auto& updates : m_frameUpdates) {
      updates.remove(id);
   }
}

void SpriteRenderer::set_object(const u32 index, const SpritePrimitive& prim)
{
   assert(index <= g_drawCallBufferSize);
   assert(m_stagingInsertionsTop < g_insertionBufferSize);
   m_stagingInsertions[m_stagingInsertionsTop].dstIndex = index;
   m_stagingInsertions[m_stagingInsertionsTop].primitive = prim;
   m_stagingInsertionsTop++;
}

void SpriteRenderer::move_object(const u32 src, const u32 dst)
{
   assert(src <= g_drawCallBufferSize);
   assert(dst <= g_drawCallBufferSize);
   assert(m_stagingRemovalsTop < g_removalBufferSize);
   m_stagingRemovals[m_stagingRemovalsTop].srcID = src;
   m_stagingRemovals[m_stagingRemovalsTop].dstID = dst;
   m_stagingRemovalsTop++;
}

void SpriteRenderer::prepare_frame(render_core::JobGraph& graph, const u32 frameIndex)
{
   // Insertions
   const auto insertions = GAPI_CHECK(graph.resources().buffer("user_interface.sprite.insertion.staging"_name, frameIndex).map_memory());
   m_stagingInsertions = &insertions.cast<SpriteWriteData>();
   m_stagingInsertionsTop = 0;

   // Removals
   const auto removals = GAPI_CHECK(graph.resources().buffer("user_interface.sprite.removal.staging"_name, frameIndex).map_memory());
   m_stagingRemovals = &removals.cast<SpriteCopyInfo>();
   m_stagingRemovalsTop = 0;

   m_frameUpdates[frameIndex].write_to_buffers(*this);

   // Write calls
   const auto insertion_dims =
      GAPI_CHECK(graph.resources().buffer("user_interface.sprite.insertion.indirect_buffer"_name, frameIndex).map_memory());
   insertion_dims.cast<Vector3u>() = {divide_rounded_up(m_stagingInsertionsTop, g_csGroupSize), 1, 1};
   const auto insertion_count = GAPI_CHECK(graph.resources().buffer("user_interface.sprite.insertion.count"_name, frameIndex).map_memory());
   insertion_count.cast<u32>() = m_stagingInsertionsTop;

   const auto removal_dims =
      GAPI_CHECK(graph.resources().buffer("user_interface.sprite.removal.indirect_buffer"_name, frameIndex).map_memory());
   removal_dims.cast<Vector3u>() = {divide_rounded_up(m_stagingRemovalsTop, g_csGroupSize), 1, 1};
   const auto removal_count = GAPI_CHECK(graph.resources().buffer("user_interface.sprite.removal.count"_name, frameIndex).map_memory());
   removal_count.cast<u32>() = m_stagingRemovalsTop;

   // Fill count
   const auto count = GAPI_CHECK(graph.resources().buffer("user_interface.sprite.count"_name, frameIndex).map_memory());
   count.cast<u32>() = m_frameUpdates[frameIndex].top_index();
}

void SpriteRenderer::build_data_update(render_core::BuildContext& ctx) const
{
   ctx.declare_buffer("user_interface.sprite.insertion"_name, sizeof(SpriteWriteData) * g_insertionBufferSize);
   ctx.declare_staging_buffer("user_interface.sprite.insertion.staging"_name, sizeof(SpriteWriteData) * g_insertionBufferSize);
   ctx.declare_staging_buffer("user_interface.sprite.insertion.count"_name, sizeof(u32));

   ctx.declare_buffer("user_interface.sprite.removal"_name, sizeof(SpriteCopyInfo) * g_removalBufferSize);
   ctx.declare_staging_buffer("user_interface.sprite.removal.staging"_name, sizeof(SpriteCopyInfo) * g_removalBufferSize);
   ctx.declare_staging_buffer("user_interface.sprite.removal.count"_name, sizeof(u32));

   ctx.declare_buffer("user_interface.sprite.draw_calls"_name, sizeof(DrawCall) * g_drawCallBufferSize);

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

   ctx.draw_indirect_with_count("user_interface.sprite.draw_calls"_external, "user_interface.sprite.count"_external, g_drawCallBufferSize,
                                sizeof(DrawCall));
}

u32 SpriteRenderer::get_texture_id(const TextureName name)
{
   const auto it = m_textureIDs.find(name);
   if (it != m_textureIDs.end()) {
      return it->second;
   }

   const auto id = static_cast<u32>(m_textures.size());
   m_textures.emplace_back(name);
   m_textureIDs.emplace(name, id);
   return id;
}

}// namespace triglav::renderer::ui
