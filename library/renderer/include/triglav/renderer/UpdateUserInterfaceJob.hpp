#pragma once

#include "triglav/Math.hpp"
#include "triglav/graphics_api/Buffer.hpp"
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
class GlyphProperties;
}// namespace triglav::render_core

namespace triglav::renderer {

struct TextInfo
{
   ui_core::Text text;
   u32 drawCallId;
   u32 glyphBufferOffset;

   Vector4 color;
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

struct RectangleData
{
   graphics_api::Buffer vsUbo;
   graphics_api::Buffer fsUbo;
   graphics_api::Buffer vertexBuffer;
};

class UpdateUserInterfaceJob
{
 public:
   using Self = UpdateUserInterfaceJob;

   static constexpr auto JobName = make_name_id("job.update_user_interface");

   explicit UpdateUserInterfaceJob(graphics_api::Device& device, render_core::GlyphCache& glyphCache, ui_core::Viewport& viewport);

   void build_job(render_core::BuildContext& ctx) const;

   void prepare_frame(render_core::JobGraph& graph, u32 frameIndex);

   void on_added_text(Name name, const ui_core::Text& text);
   void on_removed_text(Name name);
   void on_text_change_content(Name name, const ui_core::Text& text);
   void on_text_change_position(Name name, const ui_core::Text& text);
   void on_text_change_color(Name name, const ui_core::Text& text);
   void on_added_rectangle(Name name, const ui_core::Rectangle& rect);
   void on_rectangle_change_dims(Name name, const ui_core::Rectangle& rect);

   void render_ui(render_core::BuildContext& ctx);

 private:
   [[nodiscard]] const TypefaceInfo& get_typeface_info(const render_core::GlyphProperties& glyphProps);
   memory::Area allocate_vertex_section(Name text, u32 vertexCount);
   void free_vertex_section(Name textName);

   graphics_api::Device& m_device;
   render_core::GlyphCache& m_glyphCache;
   ui_core::Viewport& m_viewport;
   std::vector<Name> m_pendingTextUpdates;
   std::vector<Name> m_pendingTextRemoval;
   std::map<Name, TextInfo> m_textInfos;
   std::vector<Name> m_drawCallToTextName;
   graphics_api::Buffer m_combinedGlyphBuffer;
   u32 m_glyphOffset{};
   std::map<u64, TypefaceInfo> m_typefaceInfos;
   std::vector<render_core::TextureRef> m_atlases;
   memory::HeapAllocator m_vertexAllocator;
   std::map<Name, memory::Area> m_allocatedVertexSections;

   std::map<Name, RectangleData> m_rectangles;
   std::mutex m_textUpdateMtx;
   std::mutex m_rectUpdateMtx;

   TG_SINK(ui_core::Viewport, OnAddedText);
   TG_SINK(ui_core::Viewport, OnRemovedText);
   TG_SINK(ui_core::Viewport, OnTextChangeContent);
   TG_SINK(ui_core::Viewport, OnTextChangePosition);
   TG_SINK(ui_core::Viewport, OnTextChangeColor);
   TG_SINK(ui_core::Viewport, OnAddedRectangle);
   TG_SINK(ui_core::Viewport, OnRectangleChangeDims);
};

}// namespace triglav::renderer
