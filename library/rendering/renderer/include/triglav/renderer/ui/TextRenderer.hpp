#pragma once


#include "triglav/Logging.hpp"
#include "triglav/UpdateList.hpp"
#include "triglav/event/Delegate.hpp"
#include "triglav/memory/HeapAllocator.hpp"
#include "triglav/render_core/IRenderer.hpp"
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

struct TextUpdateInfo;

struct TextInfo
{
   ui_core::Text text;
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
   TG_DEFINE_LOG_CATEGORY(TextRenderer)
 public:
   using Self = TextRenderer;

   TextRenderer(graphics_api::Device& device, render_core::GlyphCache& glyph_cache, ui_core::Viewport& viewport,
                render_core::IRenderer& renderer);

   void on_added_text(ui_core::TextId text_id, const ui_core::Text& text);
   void on_updated_text(ui_core::TextId text_id, const ui_core::Text& text);
   void on_removed_text(ui_core::TextId text_id);

   void prepare_frame(render_core::JobGraph& graph, u32 frame_index);

   void build_data_preparation(render_core::BuildContext& ctx) const;
   void build_render_ui(render_core::BuildContext& ctx);

   void set_object(u32 index, const TextInfo& info);
   void move_object(u32 src, u32 dst);

 private:
   [[nodiscard]] const TypefaceInfo& get_typeface_info(const render_core::GlyphProperties& glyph_props);
   memory::Area allocate_vertex_section(ui_core::TextId text_id, u32 vertex_count);
   void free_vertex_section(ui_core::TextId text_id);

   graphics_api::Device& m_device;
   render_core::GlyphCache& m_glyph_cache;
   render_core::IRenderer& m_renderer;

   std::array<UpdateList<ui_core::RectId, TextInfo>, render_core::FRAMES_IN_FLIGHT_COUNT> m_frame_updates;
   std::vector<render_core::TextureRef> m_atlases;
   graphics_api::Buffer m_combined_glyph_buffer;
   u32 m_glyph_offset{};
   std::map<u64, TypefaceInfo> m_typeface_infos;
   memory::HeapAllocator m_vertex_allocator;
   std::map<ui_core::TextId, memory::Area> m_allocated_vertex_sections;

   TextUpdateInfo* m_update_infos{};
   u32 m_top_update_info = 0;
   u32* m_char_buffer{};
   std::pair<u32, u32>* m_move_buffer{};
   u32 m_top_move_index = 0;
   u32 m_current_frame_index{};
   u32 m_char_offset{};

   std::mutex m_update_mtx;

   TG_SINK(ui_core::Viewport, OnAddedText);
   TG_SINK(ui_core::Viewport, OnUpdatedText);
   TG_SINK(ui_core::Viewport, OnRemovedText);
};

}// namespace triglav::renderer::ui
