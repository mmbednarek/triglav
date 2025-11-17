#pragma once

#include "triglav/event/Delegate.hpp"
#include "triglav/memory/HeapAllocator.hpp"
#include "triglav/render_core/RenderCore.hpp"
#include "triglav/ui_core/Viewport.hpp"

#include <map>
#include <mutex>
#include <vector>

namespace triglav::render_core {
class BuildContext;
class JobGraph;
class GlyphCache;
struct GlyphProperties;
}// namespace triglav::render_core

namespace triglav::renderer::ui {

struct TextInfo
{
   ui_core::Text text;
   u32 draw_call_id;
   u32 glyph_buffer_offset;

   Vector4 color;
   Vector4 crop;
   Vector2 position;
   u32 atlas_id;

   u32 dst_vertex_offset;
   u32 dst_vertex_count;
};

struct TypefaceInfo
{
   u32 atlas_id;
   u32 glyph_buffer_offset;
};

class TextRenderer
{
 public:
   using Self = TextRenderer;

   TextRenderer(graphics_api::Device& device, render_core::GlyphCache& glyph_cache, ui_core::Viewport& viewport);

   void on_added_text(ui_core::TextId text_id, const ui_core::Text& text);
   void on_updated_text(ui_core::TextId text_id, const ui_core::Text& text);
   void on_removed_text(ui_core::TextId text_id);

   void prepare_frame(render_core::JobGraph& graph, u32 frame_index);

   void build_data_preparation(render_core::BuildContext& ctx) const;
   void build_render_ui(render_core::BuildContext& ctx);

 private:
   [[nodiscard]] const TypefaceInfo& get_typeface_info(const render_core::GlyphProperties& glyph_props);
   memory::Area allocate_vertex_section(ui_core::TextId text_id, u32 vertex_count);
   void free_vertex_section(ui_core::TextId text_id);

   graphics_api::Device& m_device;
   render_core::GlyphCache& m_glyph_cache;

   std::vector<ui_core::TextId> m_pending_text_updates;
   std::vector<ui_core::TextId> m_pending_text_removal;
   std::map<ui_core::TextId, TextInfo> m_text_infos;
   std::vector<ui_core::TextId> m_draw_call_to_text_name;
   std::vector<render_core::TextureRef> m_atlases;
   graphics_api::Buffer m_combined_glyph_buffer;
   u32 m_glyph_offset{};
   std::map<u64, TypefaceInfo> m_typeface_infos;
   memory::HeapAllocator m_vertex_allocator;
   std::map<ui_core::TextId, memory::Area> m_allocated_vertex_sections;

   std::mutex m_update_mtx;

   TG_SINK(ui_core::Viewport, OnAddedText);
   TG_SINK(ui_core::Viewport, OnUpdatedText);
   TG_SINK(ui_core::Viewport, OnRemovedText);
};

}// namespace triglav::renderer::ui
