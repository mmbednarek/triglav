#include "ui/TextRenderer.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/render_core/JobGraph.hpp"

namespace triglav::renderer::ui {

constexpr u32 g_max_text_update_count = 256;
constexpr u32 g_max_character_count = 4096;
constexpr u32 g_max_text_draw_calls = 512;
constexpr u32 g_max_text_vertices = 8192;
constexpr u32 g_max_combined_glyph_buffer_size = sizeof(render_core::GlyphInfo) * 286 * 32;
constexpr u32 g_vertex_count_per_char = 6;

using namespace name_literals;
using namespace render_core::literals;
using ui_core::TextId;

struct TextUpdateInfo
{
   Vector4 color;
   Vector4 crop;
   Vector2 position;
   u32 atlas_id;

   u32 character_offset;
   u32 character_count;
   u32 glyph_buffer_offset;

   u32 dst_draw_call;
   u32 dst_vertex_offset;
   u32 dst_vertex_count;

   u32 padding[3];
};

static_assert(sizeof(TextUpdateInfo) % 16 == 0);

struct TextDrawCall
{
   u32 vertex_count;
   u32 instance_count;
   u32 first_vertex;
   u32 first_instance;

   Vector4 color;
   Vector4 crop;
   Vector2 position;
   u32 atlas_index;

   u32 padding;
};

static_assert(sizeof(TextDrawCall) % 16 == 0);

struct TextVertex
{
   Vector2 position;
   Vector2 uv;
};

TextRenderer::TextRenderer(graphics_api::Device& device, render_core::GlyphCache& glyph_cache, ui_core::Viewport& viewport) :
    m_device(device),
    m_glyph_cache(glyph_cache),
    m_combined_glyph_buffer(GAPI_CHECK(device.create_buffer(
       graphics_api::BufferUsage::StorageBuffer | graphics_api::BufferUsage::TransferDst, g_max_combined_glyph_buffer_size))),
    m_vertex_allocator(g_max_text_vertices),
    TG_CONNECT(viewport, OnAddedText, on_added_text),
    TG_CONNECT(viewport, OnUpdatedText, on_updated_text),
    TG_CONNECT(viewport, OnRemovedText, on_removed_text)
{
}

void TextRenderer::on_added_text(const TextId text_id, const ui_core::Text& text)
{
   std::unique_lock lk{m_update_mtx};

   const auto& typeface_info = this->get_typeface_info({text.typeface_name, text.font_size});
   const auto vertex_section = this->allocate_vertex_section(text_id, static_cast<u32>(g_vertex_count_per_char * text.content.size()));

   m_text_infos.emplace(text_id, TextInfo{text, static_cast<u32>(m_draw_call_to_text_name.size()), typeface_info.glyph_buffer_offset,
                                          text.color, text.crop, text.position, typeface_info.atlas_id,
                                          static_cast<u32>(vertex_section.offset), static_cast<u32>(vertex_section.size)});
   m_draw_call_to_text_name.emplace_back(text_id);

   m_pending_text_updates.emplace_back(text_id);
}

void TextRenderer::on_removed_text(const TextId text_id)
{
   std::unique_lock lk{m_update_mtx};
   m_pending_text_removal.emplace_back(text_id);
}

void TextRenderer::on_updated_text(const TextId text_id, const ui_core::Text& text)
{
   std::unique_lock lk{m_update_mtx};

   this->free_vertex_section(text_id);
   const auto vertex_section =
      this->allocate_vertex_section(text_id, static_cast<u32>(g_vertex_count_per_char * text.content.rune_count()));

   auto& text_info = m_text_infos.at(text_id);
   text_info.text.content = text.content;
   text_info.dst_vertex_offset = static_cast<u32>(vertex_section.offset);
   text_info.dst_vertex_count = static_cast<u32>(vertex_section.size);
   text_info.position = text.position;
   text_info.crop = text.crop;
   text_info.color = text.color;

   m_pending_text_updates.emplace_back(text_id);
}

void TextRenderer::prepare_frame(render_core::JobGraph& graph, const u32 frame_index)
{
   std::unique_lock lk{m_update_mtx};

   const auto text_update_mem =
      GAPI_CHECK(graph.resources().buffer("user_interface.text_update_buffer.staging"_name, frame_index).map_memory());
   auto& text_updates = text_update_mem.cast<std::array<TextUpdateInfo, g_max_text_update_count>>();

   const auto char_buffer_mem =
      GAPI_CHECK(graph.resources().buffer("user_interface.character_buffer.staging"_name, frame_index).map_memory());
   auto& char_buffer = char_buffer_mem.cast<std::array<u32, g_max_character_count>>();

   const auto text_dispatch_buffer_mem =
      GAPI_CHECK(graph.resources().buffer("user_interface.dispatch_buffer"_name, frame_index).map_memory());
   auto& text_dispatch = text_dispatch_buffer_mem.cast<Vector3u>();

   const auto text_draw_call_count_buffer_mem =
      GAPI_CHECK(graph.resources().buffer("user_interface.text_draw_call_count"_name, frame_index).map_memory());
   auto& text_draw_call_count = text_draw_call_count_buffer_mem.cast<u32>();

   const auto text_removal_count_mem =
      GAPI_CHECK(graph.resources().buffer("user_interface.text_removal_count"_name, frame_index).map_memory());
   auto& text_removal_count = text_removal_count_mem.cast<u32>();

   const auto text_removal_buffer_mem =
      GAPI_CHECK(graph.resources().buffer("user_interface.text_removal_buffer"_name, frame_index).map_memory());
   auto& text_removal_buffer = text_removal_buffer_mem.cast<std::array<std::pair<u32, u32>, g_max_text_update_count>>();

   text_removal_count = 0;
   std::vector<u32> text_removals{};
   text_removals.reserve(text_removal_buffer.size());
   for (const auto& [id, text_name] : Enumerate(m_pending_text_removal)) {
      text_removals.emplace_back(m_text_infos[text_name].draw_call_id);
      this->free_vertex_section(text_name);
      m_text_infos.erase(text_name);
   }
   m_pending_text_removal.clear();

   std::ranges::stable_sort(text_removals);

   auto text_removal_buffer_it = text_removal_buffer.begin();

   const auto remain_count = static_cast<u32>(m_draw_call_to_text_name.size() - text_removals.size());
   u32 src_index = remain_count;
   for (const auto dst_index : text_removals) {
      while (std::ranges::binary_search(text_removals, src_index))
         ++src_index;

      if (dst_index < remain_count) {
         ui_core::TextId src_name = m_draw_call_to_text_name[src_index];
         m_text_infos[src_name].draw_call_id = dst_index;
         m_draw_call_to_text_name[dst_index] = src_name;

         *text_removal_buffer_it = {src_index, dst_index};
         ++text_removal_buffer_it;
         ++src_index;
         ++text_removal_count;
      }
   }

   m_draw_call_to_text_name.resize(remain_count);

   u32 char_offset = 0;

   text_dispatch = {static_cast<u32>(m_pending_text_updates.size()), 1, 1};
   text_draw_call_count = static_cast<u32>(m_draw_call_to_text_name.size());

   for (const auto [index, text_name] : Enumerate(m_pending_text_updates)) {
      if (index >= g_max_text_update_count)
         break;

      auto text_info_it = m_text_infos.find(text_name);
      if (text_info_it == m_text_infos.end())
         continue;

      const auto& text_info = text_info_it->second;
      auto& dst_update = text_updates.at(index);

      dst_update.dst_draw_call = text_info.draw_call_id;
      dst_update.character_offset = char_offset;
      dst_update.character_count =
         font::Charset::European.encode_string_to(text_info.text.content.view(), char_buffer.begin() + char_offset);
      dst_update.color = text_info.color;
      dst_update.crop = text_info.crop;
      dst_update.position = text_info.position;
      dst_update.atlas_id = text_info.atlas_id;
      dst_update.glyph_buffer_offset = text_info.glyph_buffer_offset;
      dst_update.dst_vertex_offset = text_info.dst_vertex_offset;
      dst_update.dst_vertex_count = text_info.dst_vertex_count;
      char_offset += dst_update.character_count;
   }

   if (m_pending_text_updates.size() <= g_max_text_update_count) {
      m_pending_text_updates.clear();
   } else {
      std::copy(m_pending_text_updates.begin() + g_max_text_update_count, m_pending_text_updates.end(), m_pending_text_updates.begin());
      m_pending_text_updates.resize(m_pending_text_updates.size() - g_max_text_update_count);
   }
}

void TextRenderer::build_data_preparation(render_core::BuildContext& ctx) const
{
   ctx.declare_buffer("user_interface.text_update_buffer"_name, sizeof(TextUpdateInfo) * g_max_text_update_count);
   ctx.declare_staging_buffer("user_interface.text_update_buffer.staging"_name, sizeof(TextUpdateInfo) * g_max_text_update_count);

   ctx.declare_buffer("user_interface.character_buffer"_name, sizeof(u32) * g_max_character_count);
   ctx.declare_staging_buffer("user_interface.character_buffer.staging"_name, sizeof(u32) * g_max_character_count);

   ctx.declare_staging_buffer("user_interface.dispatch_buffer"_name, sizeof(Vector3u));
   ctx.declare_staging_buffer("user_interface.text_draw_call_count"_name, sizeof(u32));
   ctx.declare_staging_buffer("user_interface.text_removal_buffer"_name, sizeof(std::pair<u32, u32>) * g_max_text_update_count);
   ctx.declare_staging_buffer("user_interface.text_removal_count"_name, sizeof(u32));

   ctx.declare_buffer("user_interface.text_draw_call_buffer"_name, sizeof(TextDrawCall) * g_max_text_draw_calls);
   ctx.declare_buffer("user_interface.text_vertex_buffer"_name, sizeof(TextVertex) * g_max_text_vertices);

   // Copy buffers
   ctx.copy_buffer("user_interface.text_update_buffer.staging"_name, "user_interface.text_update_buffer"_name);
   ctx.copy_buffer("user_interface.character_buffer.staging"_name, "user_interface.character_buffer"_name);
   ctx.copy_buffer("user_interface.text_draw_call_buffer"_last_frame, "user_interface.text_draw_call_buffer"_name);
   ctx.copy_buffer("user_interface.text_vertex_buffer"_last_frame, "user_interface.text_vertex_buffer"_name);

   // Remove pending texts
   ctx.bind_compute_shader("text/removal.cshader"_rc);

   ctx.bind_uniform_buffer(0, "user_interface.text_removal_count"_name);
   ctx.bind_storage_buffer(1, "user_interface.text_removal_buffer"_name);
   ctx.bind_storage_buffer(2, "user_interface.text_draw_call_buffer"_name);

   ctx.dispatch({1, 1, 1});

   // Dispatch buffer
   ctx.bind_compute_shader("text/generate_geometry.cshader"_rc);

   // input
   ctx.bind_storage_buffer(0, "user_interface.text_update_buffer"_name);
   ctx.bind_storage_buffer(1, "user_interface.character_buffer"_name);
   ctx.bind_storage_buffer(2, &this->m_combined_glyph_buffer);

   // output
   ctx.bind_storage_buffer(3, "user_interface.text_draw_call_buffer"_name);
   ctx.bind_storage_buffer(4, "user_interface.text_vertex_buffer"_name);

   ctx.dispatch_indirect("user_interface.dispatch_buffer"_name);

   ctx.export_buffer("user_interface.text_draw_call_buffer"_name, graphics_api::PipelineStage::DrawIndirect,
                     graphics_api::BufferAccess::IndirectCmdRead,
                     graphics_api::BufferUsage::StorageBuffer | graphics_api::BufferUsage::Indirect);
   ctx.export_buffer("user_interface.text_vertex_buffer"_name, graphics_api::PipelineStage::VertexInput,
                     graphics_api::BufferAccess::VertexRead, graphics_api::BufferUsage::VertexBuffer);
   ctx.export_buffer("user_interface.text_draw_call_count"_name, graphics_api::PipelineStage::DrawIndirect,
                     graphics_api::BufferAccess::IndirectCmdRead, graphics_api::BufferUsage::Indirect);
}

void TextRenderer::build_render_ui(render_core::BuildContext& ctx)
{
   ctx.bind_vertex_shader("text/render.vshader"_rc);

   ctx.push_constant(ctx.screen_size());

   ctx.bind_storage_buffer(0, "user_interface.text_draw_call_buffer"_external);

   ctx.bind_fragment_shader("text/render.fshader"_rc);

   ctx.bind_sampled_texture_array(1, m_atlases);

   render_core::VertexLayout layout(sizeof(TextVertex));
   layout.add("position"_name, GAPI_FORMAT(RG, Float32), offsetof(TextVertex, position));
   layout.add("uv"_name, GAPI_FORMAT(RG, Float32), offsetof(TextVertex, uv));
   ctx.bind_vertex_layout(layout);

   ctx.bind_vertex_buffer("user_interface.text_vertex_buffer"_external);

   ctx.draw_indirect_with_count("user_interface.text_draw_call_buffer"_external, "user_interface.text_draw_call_count"_external,
                                g_max_text_draw_calls, sizeof(TextDrawCall));
}

const TypefaceInfo& TextRenderer::get_typeface_info(const render_core::GlyphProperties& glyph_props)
{
   if (const auto it = m_typeface_infos.find(glyph_props.hash()); it != m_typeface_infos.end()) {
      return it->second;
   }

   auto& glyph_atlas = m_glyph_cache.find_glyph_atlas(glyph_props);

   const auto atlas_id = static_cast<u32>(m_atlases.size());
   m_atlases.emplace_back(&glyph_atlas.texture());

   const auto transfer_cmd_list = GAPI_CHECK(m_device.create_command_list(graphics_api::WorkType::Transfer));

   GAPI_CHECK_STATUS(transfer_cmd_list.begin(graphics_api::SubmitType::OneTime));
   transfer_cmd_list.copy_buffer(glyph_atlas.storage_buffer(), m_combined_glyph_buffer, 0, m_glyph_offset,
                                 static_cast<u32>(glyph_atlas.storage_buffer().size()));
   GAPI_CHECK_STATUS(transfer_cmd_list.finish());

   GAPI_CHECK_STATUS(m_device.submit_command_list_one_time(transfer_cmd_list));

   auto [new_it, ok] = m_typeface_infos.emplace(glyph_props.hash(),
                                                TypefaceInfo{atlas_id, static_cast<u32>(m_glyph_offset / sizeof(render_core::GlyphInfo))});
   assert(ok);

   m_glyph_offset += static_cast<u32>(glyph_atlas.storage_buffer().size());

   return new_it->second;
}

memory::Area TextRenderer::allocate_vertex_section(const TextId text_id, const u32 vertex_count)
{
   const auto section = m_vertex_allocator.allocate(vertex_count);
   assert(section.has_value());

   memory::Area result{vertex_count, *section};
   m_allocated_vertex_sections.emplace(text_id, result);

   return result;
}

void TextRenderer::free_vertex_section(const TextId text_id)
{
   m_vertex_allocator.free(m_allocated_vertex_sections.at(text_id));
   m_allocated_vertex_sections.erase(text_id);
}

}// namespace triglav::renderer::ui
