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
   u32 drawCallId;
   u32 glyphBufferOffset;

   Vector4 color;
   Vector4 crop;
   Vector2 position;
   u32 atlasId;

   u32 dstVertexOffset;
   u32 dstVertexCount;
};

struct TypefaceInfo
{
   u32 atlasId;
   u32 glyphBufferOffset;
};

class TextRenderer
{
 public:
   using Self = TextRenderer;

   TextRenderer(graphics_api::Device& device, render_core::GlyphCache& glyphCache, ui_core::Viewport& viewport);

   void on_added_text(ui_core::TextId textId, const ui_core::Text& text);
   void on_updated_text(ui_core::TextId textId, const ui_core::Text& text);
   void on_removed_text(ui_core::TextId textId);

   void prepare_frame(render_core::JobGraph& graph, u32 frameIndex);

   void build_data_preparation(render_core::BuildContext& ctx) const;
   void build_render_ui(render_core::BuildContext& ctx);

 private:
   [[nodiscard]] const TypefaceInfo& get_typeface_info(const render_core::GlyphProperties& glyphProps);
   memory::Area allocate_vertex_section(ui_core::TextId textId, u32 vertexCount);
   void free_vertex_section(ui_core::TextId textId);

   graphics_api::Device& m_device;
   render_core::GlyphCache& m_glyphCache;

   std::vector<ui_core::TextId> m_pendingTextUpdates;
   std::vector<ui_core::TextId> m_pendingTextRemoval;
   std::map<ui_core::TextId, TextInfo> m_textInfos;
   std::vector<ui_core::TextId> m_drawCallToTextName;
   std::vector<render_core::TextureRef> m_atlases;
   graphics_api::Buffer m_combinedGlyphBuffer;
   u32 m_glyphOffset{};
   std::map<u64, TypefaceInfo> m_typefaceInfos;
   memory::HeapAllocator m_vertexAllocator;
   std::map<ui_core::TextId, memory::Area> m_allocatedVertexSections;

   std::mutex m_updateMtx;

   TG_SINK(ui_core::Viewport, OnAddedText);
   TG_SINK(ui_core::Viewport, OnUpdatedText);
   TG_SINK(ui_core::Viewport, OnRemovedText);
};

}// namespace triglav::renderer::ui
