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

TextRenderer::TextRenderer(graphics_api::Device& device, render_core::GlyphCache& glyph_cache, ui_core::Viewport& viewport,
                           render_core::IRenderer& renderer) :
    m_device(device),
    m_glyph_cache(glyph_cache),
    m_renderer(renderer),
    m_combined_glyph_buffer(GAPI_CHECK(device.create_buffer(
       graphics_api::BufferUsage::StorageBuffer | graphics_api::BufferUsage::TransferDst, g_max_combined_glyph_buffer_size))),
    m_vertex_allocator{memory::HeapAllocator{g_max_text_vertices}},
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

   for (auto& updates : m_frame_updates) {
      updates.add_or_update(text_id,
                            TextInfo{text, typeface_info.glyph_buffer_offset, text.color, text.crop, text.position, typeface_info.atlas_id,
                                     static_cast<u32>(vertex_section.offset), static_cast<u32>(vertex_section.size)});
   }
}

void TextRenderer::on_removed_text(const TextId text_id)
{
   std::unique_lock lk{m_update_mtx};
   for (auto& updates : m_frame_updates) {
      updates.remove(text_id);
   }
   this->free_vertex_section(text_id);
}

void TextRenderer::on_updated_text(const TextId text_id, const ui_core::Text& text)
{
   this->on_added_text(text_id, text);
}

void TextRenderer::prepare_frame(render_core::JobGraph& graph, const u32 frame_index)
{
   std::unique_lock lk{m_update_mtx};

   const auto text_update_mem =
      GAPI_CHECK(graph.resources().buffer("user_interface.text_update_buffer.staging"_name, frame_index).map_memory());
   m_update_infos = static_cast<TextUpdateInfo*>(*text_update_mem);

   const auto char_buffer_mem =
      GAPI_CHECK(graph.resources().buffer("user_interface.character_buffer.staging"_name, frame_index).map_memory());
   m_char_buffer = static_cast<u32*>(*char_buffer_mem);

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
   m_move_buffer = static_cast<std::pair<u32, u32>*>(*text_removal_buffer_mem);

   m_top_update_info = 0;
   m_top_move_index = 0;
   m_char_offset = 0;
   m_current_frame_index = frame_index;

   m_frame_updates[frame_index].write_to_buffers(*this);

   text_draw_call_count = m_frame_updates[frame_index].top_index();
   text_removal_count = m_top_move_index;
   text_dispatch = {m_top_update_info, 1, 1};
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

   // Dispatch buffer
   ctx.bind_compute_shader("shader/ui/text_geometry_gen.cshader"_rc);

   // input
   ctx.bind_storage_buffer(0, "user_interface.text_update_buffer"_name);
   ctx.bind_storage_buffer(1, "user_interface.character_buffer"_name);
   ctx.bind_storage_buffer(2, &this->m_combined_glyph_buffer);

   // output
   ctx.bind_storage_buffer(3, "user_interface.text_draw_call_buffer"_name);
   ctx.bind_storage_buffer(4, "user_interface.text_vertex_buffer"_name);

   ctx.dispatch_indirect("user_interface.dispatch_buffer"_name);

   // Remove pending texts
   ctx.bind_compute_shader("shader/ui/text_removal.cshader"_rc);

   ctx.bind_uniform_buffer(0, "user_interface.text_removal_count"_name);
   ctx.bind_storage_buffer(1, "user_interface.text_removal_buffer"_name);
   ctx.bind_storage_buffer(2, "user_interface.text_draw_call_buffer"_name);

   ctx.dispatch({1, 1, 1});

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
   ctx.bind_vertex_shader("shader/ui/text_render.vshader"_rc);

   ctx.push_constant(ctx.screen_size());

   ctx.bind_storage_buffer(0, "user_interface.text_draw_call_buffer"_external);

   ctx.bind_fragment_shader("shader/ui/text_render.fshader"_rc);

   ctx.bind_sampled_texture_array(1, m_atlases);

   render_core::VertexLayout layout(sizeof(TextVertex));
   layout.add("position"_name, GAPI_FORMAT(RG, Float32), offsetof(TextVertex, position));
   layout.add("uv"_name, GAPI_FORMAT(RG, Float32), offsetof(TextVertex, uv));
   ctx.bind_vertex_layout(layout);

   ctx.bind_vertex_buffer("user_interface.text_vertex_buffer"_external);

   ctx.draw_indirect_with_count("user_interface.text_draw_call_buffer"_external, "user_interface.text_draw_call_count"_external,
                                g_max_text_draw_calls, sizeof(TextDrawCall));
}

void TextRenderer::set_object(const u32 index, const TextInfo& info)
{
   const auto char_count = font::Charset::European.encode_string_to(info.text.content.view(), m_char_buffer + m_char_offset);
   m_update_infos[m_top_update_info++] = TextUpdateInfo{
      .color = info.color,
      .crop = info.crop,
      .position = info.position,
      .atlas_id = info.atlas_id,
      .character_offset = m_char_offset,
      .character_count = char_count,
      .glyph_buffer_offset = info.glyph_buffer_offset,
      .dst_draw_call = index,
      .dst_vertex_offset = info.dst_vertex_offset,
      .dst_vertex_count = info.dst_vertex_count,
   };
   m_char_offset += char_count;
   assert(m_char_offset < g_max_character_count);
}

void TextRenderer::move_object(const u32 src, const u32 dst)
{
   assert(m_top_update_info <= g_max_text_update_count);
   m_move_buffer[m_top_move_index++] = {src, dst};
}

const TypefaceInfo& TextRenderer::get_typeface_info(const render_core::GlyphProperties& glyph_props)
{
   if (const auto it = m_typeface_infos.find(glyph_props.hash()); it != m_typeface_infos.end()) {
      return it->second;
   }

   m_renderer.recreate_render_jobs();

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
   if (m_allocated_vertex_sections.contains(text_id)) {
      const auto& sec = m_allocated_vertex_sections.at(text_id);
      if (sec.size == vertex_count)
         return sec;

      m_vertex_allocator.free(sec);
   }

   const auto section = m_vertex_allocator.allocate(vertex_count);
   assert(section.has_value());

   log_debug("allocated {} bytes at {}", vertex_count, section.value());

   const memory::Area result{vertex_count, *section};
   m_allocated_vertex_sections[text_id] = result;

   return result;
}

void TextRenderer::free_vertex_section(const TextId text_id)
{
   const auto section = m_allocated_vertex_sections.at(text_id);
   log_debug("freed {} bytes at {}", section.size, section.size);
   m_vertex_allocator.free(section);
   m_allocated_vertex_sections.erase(text_id);
}

}// namespace triglav::renderer::ui
