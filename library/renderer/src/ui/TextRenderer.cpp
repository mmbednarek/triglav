#include "ui/TextRenderer.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/render_core/JobGraph.hpp"

#include <spdlog/spdlog.h>

namespace triglav::renderer::ui {

constexpr u32 g_maxTextUpdateCount = 256;
constexpr u32 g_maxCharacterCount = 4096;
constexpr u32 g_maxTextDrawCalls = 512;
constexpr u32 g_maxTextVertices = 8192;
constexpr u32 g_maxCombinedGlyphBufferSize = sizeof(render_core::GlyphInfo) * 286 * 32;
constexpr u32 g_vertexCountPerChar = 6;

using namespace name_literals;
using namespace render_core::literals;

struct TextUpdateInfo
{
   Vector4 color;
   Vector4 crop;
   Vector2 position;
   u32 atlasId;

   u32 characterOffset;
   u32 characterCount;
   u32 glyphBufferOffset;

   u32 dstDrawCall;
   u32 dstVertexOffset;
   u32 dstVertexCount;

   u32 padding[3];
};

static_assert(sizeof(TextUpdateInfo) % 16 == 0);

struct TextDrawCall
{
   u32 vertexCount;
   u32 instanceCount;
   u32 firstVertex;
   u32 firstInstance;

   Vector4 color;
   Vector4 crop;
   Vector2 position;
   u32 atlasIndex;

   u32 padding;
};

static_assert(sizeof(TextDrawCall) % 16 == 0);

struct TextVertex
{
   Vector2 position;
   Vector2 uv;
};

TextRenderer::TextRenderer(graphics_api::Device& device, render_core::GlyphCache& glyphCache, ui_core::Viewport& viewport) :
    m_device(device),
    m_glyphCache(glyphCache),
    m_viewport(viewport),
    m_combinedGlyphBuffer(GAPI_CHECK(device.create_buffer(graphics_api::BufferUsage::StorageBuffer | graphics_api::BufferUsage::TransferDst,
                                                          g_maxCombinedGlyphBufferSize))),
    m_vertexAllocator(g_maxTextVertices),
    TG_CONNECT(viewport, OnAddedText, on_added_text),
    TG_CONNECT(viewport, OnRemovedText, on_removed_text),
    TG_CONNECT(viewport, OnTextChangeContent, on_text_change_content),
    TG_CONNECT(viewport, OnTextChangePosition, on_text_change_position),
    TG_CONNECT(viewport, OnTextChangeCrop, on_text_change_crop),
    TG_CONNECT(viewport, OnTextChangeColor, on_text_change_color)
{
}

void TextRenderer::on_added_text(const Name name, const ui_core::Text& text)
{
   std::unique_lock lk{m_updateMtx};

   const auto& typefaceInfo = this->get_typeface_info({text.typefaceName, text.fontSize});
   const auto vertexSection = this->allocate_vertex_section(name, static_cast<u32>(g_vertexCountPerChar * text.content.size()));

   m_textInfos.emplace(name, TextInfo{text, static_cast<u32>(m_drawCallToTextName.size()), typefaceInfo.glyphBufferOffset, text.color,
                                      text.crop, text.position, typefaceInfo.atlasId, static_cast<u32>(vertexSection.offset),
                                      static_cast<u32>(vertexSection.size)});
   m_drawCallToTextName.emplace_back(name);

   m_pendingTextUpdates.emplace_back(name);
}

void TextRenderer::on_removed_text(const Name name)
{
   std::unique_lock lk{m_updateMtx};
   m_pendingTextRemoval.emplace_back(name);
}

void TextRenderer::on_text_change_content(const Name name, const ui_core::Text& text)
{
   std::unique_lock lk{m_updateMtx};

   this->free_vertex_section(name);
   const auto vertexSection = this->allocate_vertex_section(name, static_cast<u32>(g_vertexCountPerChar * text.content.rune_count()));

   auto& textInfo = m_textInfos.at(name);
   textInfo.text.content = text.content;
   textInfo.dstVertexOffset = static_cast<u32>(vertexSection.offset);
   textInfo.dstVertexCount = static_cast<u32>(vertexSection.size);

   m_pendingTextUpdates.emplace_back(name);
}

void TextRenderer::on_text_change_position(const Name name, const ui_core::Text& text)
{
   std::unique_lock lk{m_updateMtx};
   auto& textInfo = m_textInfos.at(name);
   textInfo.position = text.position;
   m_pendingTextUpdates.emplace_back(name);
}

void TextRenderer::on_text_change_crop(Name name, const ui_core::Text& text)
{
   std::unique_lock lk{m_updateMtx};
   auto& textInfo = m_textInfos.at(name);
   textInfo.crop = text.crop;
   m_pendingTextUpdates.emplace_back(name);
}

void TextRenderer::on_text_change_color(const Name name, const ui_core::Text& text)
{
   std::unique_lock lk{m_updateMtx};
   auto& textInfo = m_textInfos.at(name);
   textInfo.color = text.color;
   m_pendingTextUpdates.emplace_back(name);
}

void TextRenderer::prepare_frame(render_core::JobGraph& graph, const u32 frameIndex)
{
   std::unique_lock lk{m_updateMtx};

   const auto textUpdateMem =
      GAPI_CHECK(graph.resources().buffer("user_interface.text_update_buffer.staging"_name, frameIndex).map_memory());
   auto& textUpdates = textUpdateMem.cast<std::array<TextUpdateInfo, g_maxTextUpdateCount>>();

   const auto charBufferMem = GAPI_CHECK(graph.resources().buffer("user_interface.character_buffer.staging"_name, frameIndex).map_memory());
   auto& charBuffer = charBufferMem.cast<std::array<u32, g_maxCharacterCount>>();

   const auto textDispatchBufferMem = GAPI_CHECK(graph.resources().buffer("user_interface.dispatch_buffer"_name, frameIndex).map_memory());
   auto& textDispatch = textDispatchBufferMem.cast<Vector3u>();

   const auto textDrawCallCountBufferMem =
      GAPI_CHECK(graph.resources().buffer("user_interface.text_draw_call_count"_name, frameIndex).map_memory());
   auto& textDrawCallCount = textDrawCallCountBufferMem.cast<u32>();

   const auto textRemovalCountMem = GAPI_CHECK(graph.resources().buffer("user_interface.text_removal_count"_name, frameIndex).map_memory());
   auto& textRemovalCount = textRemovalCountMem.cast<u32>();

   const auto textRemovalBufferMem =
      GAPI_CHECK(graph.resources().buffer("user_interface.text_removal_buffer"_name, frameIndex).map_memory());
   auto& textRemovalBuffer = textRemovalBufferMem.cast<std::array<std::pair<u32, u32>, g_maxTextUpdateCount>>();

   textRemovalCount = 0;
   std::vector<u32> textRemovals{};
   textRemovals.reserve(textRemovalBuffer.size());
   for (const auto& [id, textName] : Enumerate(m_pendingTextRemoval)) {
      textRemovals.emplace_back(m_textInfos[textName].drawCallId);
      this->free_vertex_section(textName);
      m_textInfos.erase(textName);
   }
   m_pendingTextRemoval.clear();

   std::ranges::stable_sort(textRemovals);

   auto textRemovalBufferIt = textRemovalBuffer.begin();

   const auto remainCount = static_cast<u32>(m_drawCallToTextName.size() - textRemovals.size());
   u32 srcIndex = remainCount;
   for (const auto dstIndex : textRemovals) {
      while (std::ranges::binary_search(textRemovals, srcIndex))
         ++srcIndex;

      if (dstIndex < remainCount) {
         Name srcName = m_drawCallToTextName[srcIndex];
         m_textInfos[srcName].drawCallId = dstIndex;
         m_drawCallToTextName[dstIndex] = srcName;

         *textRemovalBufferIt = {srcIndex, dstIndex};
         ++textRemovalBufferIt;
         ++srcIndex;
         ++textRemovalCount;
      }
   }

   m_drawCallToTextName.resize(remainCount);

   u32 charOffset = 0;

   textDispatch = {static_cast<u32>(m_pendingTextUpdates.size()), 1, 1};
   textDrawCallCount = static_cast<u32>(m_drawCallToTextName.size());

   for (const auto [index, textName] : Enumerate(m_pendingTextUpdates)) {
      if (index >= g_maxTextUpdateCount)
         break;

      const auto& textInfo = m_textInfos.at(textName);
      auto& dstUpdate = textUpdates.at(index);

      dstUpdate.dstDrawCall = textInfo.drawCallId;
      dstUpdate.characterOffset = charOffset;
      dstUpdate.characterCount = font::Charset::European.encode_string_to(textInfo.text.content.view(), charBuffer.begin() + charOffset);
      dstUpdate.color = textInfo.color;
      dstUpdate.crop = textInfo.crop;
      dstUpdate.position = textInfo.position;
      dstUpdate.atlasId = textInfo.atlasId;
      dstUpdate.glyphBufferOffset = textInfo.glyphBufferOffset;
      dstUpdate.dstVertexOffset = textInfo.dstVertexOffset;
      dstUpdate.dstVertexCount = textInfo.dstVertexCount;
      charOffset += dstUpdate.characterCount;
   }

   if (m_pendingTextUpdates.size() <= g_maxTextUpdateCount) {
      m_pendingTextUpdates.clear();
   } else {
      std::copy(m_pendingTextUpdates.begin() + g_maxTextUpdateCount, m_pendingTextUpdates.end(), m_pendingTextUpdates.begin());
      m_pendingTextUpdates.resize(m_pendingTextUpdates.size() - g_maxTextUpdateCount);
   }
}

void TextRenderer::build_data_preparation(render_core::BuildContext& ctx) const
{
   ctx.declare_buffer("user_interface.text_update_buffer"_name, sizeof(TextUpdateInfo) * g_maxTextUpdateCount);
   ctx.declare_staging_buffer("user_interface.text_update_buffer.staging"_name, sizeof(TextUpdateInfo) * g_maxTextUpdateCount);

   ctx.declare_buffer("user_interface.character_buffer"_name, sizeof(u32) * g_maxCharacterCount);
   ctx.declare_staging_buffer("user_interface.character_buffer.staging"_name, sizeof(u32) * g_maxCharacterCount);

   ctx.declare_staging_buffer("user_interface.dispatch_buffer"_name, sizeof(Vector3u));
   ctx.declare_staging_buffer("user_interface.text_draw_call_count"_name, sizeof(u32));
   ctx.declare_staging_buffer("user_interface.text_removal_buffer"_name, sizeof(std::pair<u32, u32>) * g_maxTextUpdateCount);
   ctx.declare_staging_buffer("user_interface.text_removal_count"_name, sizeof(u32));

   ctx.declare_buffer("user_interface.text_draw_call_buffer"_name, sizeof(TextDrawCall) * g_maxTextDrawCalls);
   ctx.declare_buffer("user_interface.text_vertex_buffer"_name, sizeof(TextVertex) * g_maxTextVertices);

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
   ctx.bind_storage_buffer(2, &this->m_combinedGlyphBuffer);

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
                                g_maxTextDrawCalls, sizeof(TextDrawCall));
}

const TypefaceInfo& TextRenderer::get_typeface_info(const render_core::GlyphProperties& glyphProps)
{
   if (const auto it = m_typefaceInfos.find(glyphProps.hash()); it != m_typefaceInfos.end()) {
      return it->second;
   }

   auto& glyphAtlas = m_glyphCache.find_glyph_atlas(glyphProps);

   const auto atlasId = static_cast<u32>(m_atlases.size());
   m_atlases.emplace_back(&glyphAtlas.texture());

   const auto transferCmdList = GAPI_CHECK(m_device.create_command_list(graphics_api::WorkType::Transfer));

   GAPI_CHECK_STATUS(transferCmdList.begin(graphics_api::SubmitType::OneTime));
   transferCmdList.copy_buffer(glyphAtlas.storage_buffer(), m_combinedGlyphBuffer, 0, m_glyphOffset,
                               static_cast<u32>(glyphAtlas.storage_buffer().size()));
   GAPI_CHECK_STATUS(transferCmdList.finish());

   GAPI_CHECK_STATUS(m_device.submit_command_list_one_time(transferCmdList));

   auto [newIt, ok] =
      m_typefaceInfos.emplace(glyphProps.hash(), TypefaceInfo{atlasId, static_cast<u32>(m_glyphOffset / sizeof(render_core::GlyphInfo))});
   assert(ok);

   m_glyphOffset += static_cast<u32>(glyphAtlas.storage_buffer().size());

   return newIt->second;
}

memory::Area TextRenderer::allocate_vertex_section(const Name text, const u32 vertexCount)
{
   const auto section = m_vertexAllocator.allocate(vertexCount);
   assert(section.has_value());

   memory::Area result{vertexCount, *section};
   m_allocatedVertexSections.emplace(text, result);

   return result;
}

void TextRenderer::free_vertex_section(const Name textName)
{
   m_vertexAllocator.free(m_allocatedVertexSections.at(textName));
   m_allocatedVertexSections.erase(textName);
}

}// namespace triglav::renderer::ui
