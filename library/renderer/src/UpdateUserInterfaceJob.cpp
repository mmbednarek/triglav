#include "UpdateUserInterfaceJob.hpp"

#include "GlyphCache.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/font/Charset.hpp"
#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/JobGraph.hpp"

#include <spdlog/spdlog.h>

namespace triglav::renderer {

using namespace name_literals;
using namespace render_core::literals;

struct TextUpdateInfo
{
   Vector4 color;
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

constexpr u32 g_maxTextUpdateCount = 256;
constexpr u32 g_maxCharacterCount = 4096;
constexpr u32 g_maxTextDrawCalls = 512;
constexpr u32 g_maxTextVertices = 8192;
constexpr u32 g_maxCombinedGlyphBufferSize = sizeof(render_core::GlyphInfo) * 286 * 32;
constexpr u32 g_vertexCountPerChar = 6;

UpdateUserInterfaceJob::UpdateUserInterfaceJob(graphics_api::Device& device, GlyphCache& glyphCache, ui_core::Viewport& viewport) :
    m_device(device),
    m_glyphCache(glyphCache),
    m_combinedGlyphBuffer(GAPI_CHECK(device.create_buffer(graphics_api::BufferUsage::StorageBuffer | graphics_api::BufferUsage::TransferDst,
                                                          g_maxCombinedGlyphBufferSize))),
    TG_CONNECT(viewport, OnAddedText, on_added_text),
    TG_CONNECT(viewport, OnTextChangeContent, on_text_change_content),
    TG_CONNECT(viewport, OnAddedRectangle, on_added_rectangle)
{
}

void UpdateUserInterfaceJob::build_job(render_core::BuildContext& ctx) const
{
   ctx.declare_buffer("user_interface.text_update_buffer"_name, sizeof(TextUpdateInfo) * g_maxTextUpdateCount);
   ctx.declare_staging_buffer("user_interface.text_update_buffer.staging"_name, sizeof(TextUpdateInfo) * g_maxTextUpdateCount);

   ctx.declare_buffer("user_interface.character_buffer"_name, sizeof(u32) * g_maxCharacterCount);
   ctx.declare_staging_buffer("user_interface.character_buffer.staging"_name, sizeof(u32) * g_maxCharacterCount);

   ctx.declare_staging_buffer("user_interface.dispatch_buffer"_name, sizeof(Vector3u));
   ctx.declare_staging_buffer("user_interface.text_draw_call_count"_name, sizeof(u32));

   ctx.declare_buffer("user_interface.text_draw_call_buffer"_name, sizeof(TextDrawCall) * g_maxTextDrawCalls);
   ctx.declare_buffer("user_interface.text_vertex_buffer"_name, sizeof(TextVertex) * g_maxTextVertices);

   // Copy buffers
   ctx.copy_buffer("user_interface.text_update_buffer.staging"_name, "user_interface.text_update_buffer"_name);
   ctx.copy_buffer("user_interface.character_buffer.staging"_name, "user_interface.character_buffer"_name);
   ctx.copy_buffer("user_interface.text_draw_call_buffer"_last_frame, "user_interface.text_draw_call_buffer"_name);
   ctx.copy_buffer("user_interface.text_vertex_buffer"_last_frame, "user_interface.text_vertex_buffer"_name);

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

   ctx.export_buffer("user_interface.text_draw_call_buffer"_name, graphics_api::PipelineStage::VertexShader,
                     graphics_api::BufferAccess::ShaderRead, graphics_api::BufferUsage::StorageBuffer);
   ctx.export_buffer("user_interface.text_vertex_buffer"_name, graphics_api::PipelineStage::VertexInput,
                     graphics_api::BufferAccess::VertexRead, graphics_api::BufferUsage::VertexBuffer);
   ctx.export_buffer("user_interface.text_draw_call_count"_name, graphics_api::PipelineStage::DrawIndirect,
                     graphics_api::BufferAccess::IndirectCmdRead, graphics_api::BufferUsage::Indirect);
}

void UpdateUserInterfaceJob::prepare_frame(render_core::JobGraph& graph, const u32 frameIndex)
{
   const auto textUpdateMem =
      GAPI_CHECK(graph.resources().buffer("user_interface.text_update_buffer.staging"_name, frameIndex).map_memory());
   auto& textUpdates = textUpdateMem.cast<std::array<TextUpdateInfo, g_maxTextUpdateCount>>();

   auto charBufferMem = GAPI_CHECK(graph.resources().buffer("user_interface.character_buffer.staging"_name, frameIndex).map_memory());
   auto& charBuffer = charBufferMem.cast<std::array<u32, g_maxCharacterCount>>();

   const auto textDispatchBufferMem = GAPI_CHECK(graph.resources().buffer("user_interface.dispatch_buffer"_name, frameIndex).map_memory());
   auto& textDispatch = textDispatchBufferMem.cast<Vector3u>();

   const auto textDrawCallCountBufferMem =
      GAPI_CHECK(graph.resources().buffer("user_interface.text_draw_call_count"_name, frameIndex).map_memory());
   auto& textDrawCallCount = textDrawCallCountBufferMem.cast<u32>();

   u32 charOffset = 0;

   textDispatch = {m_pendingTextUpdates.size(), 1, 1};
   textDrawCallCount = m_topTextDrawCallId;

   for (const auto [index, textName] : Enumerate(m_pendingTextUpdates)) {
      const auto& textInfo = m_textInfos.at(textName);
      auto& dstUpdate = textUpdates.at(index);

      dstUpdate.dstDrawCall = textInfo.drawCallId;
      dstUpdate.characterOffset = charOffset;
      dstUpdate.characterCount = font::Charset::European.encode_string_to(textInfo.text.content, charBuffer.begin() + charOffset);
      dstUpdate.color = textInfo.color;
      dstUpdate.position = textInfo.position;
      dstUpdate.atlasId = textInfo.atlasId;
      dstUpdate.glyphBufferOffset = textInfo.glyphBufferOffset;
      dstUpdate.dstVertexOffset = textInfo.dstVertexOffset;
      dstUpdate.dstVertexCount = textInfo.dstVertexCount;
      charOffset += dstUpdate.characterCount;
   }

   m_pendingTextUpdates.clear();
}

void UpdateUserInterfaceJob::on_added_text(const Name name, const ui_core::Text& text)
{
   const auto& typefaceInfo = this->get_typeface_info({text.typefaceName, text.fontSize});
   const auto vertexSection = this->allocate_vertex_section(name, g_vertexCountPerChar * text.content.size());

   m_textInfos.emplace(name, TextInfo{text, m_topTextDrawCallId, typefaceInfo.glyphBufferOffset, text.color, text.position,
                                      typefaceInfo.atlasId, vertexSection.vertexOffset, vertexSection.vertexCount});
   ++m_topTextDrawCallId;

   m_pendingTextUpdates.emplace_back(name);
}

void UpdateUserInterfaceJob::on_text_change_content(const Name name, const ui_core::Text& text)
{
   this->free_vertex_section(name);
   const auto vertexSection = this->allocate_vertex_section(name, g_vertexCountPerChar * text.content.size());

   auto& textInfo = m_textInfos.at(name);
   textInfo.text.content = text.content;
   textInfo.dstVertexOffset = vertexSection.vertexOffset;
   textInfo.dstVertexCount = vertexSection.vertexCount;

   m_pendingTextUpdates.emplace_back(name);
}

void UpdateUserInterfaceJob::on_added_rectangle(const Name /*name*/, const ui_core::Rectangle& rect)
{
   const std::array vertices{
      Vector2{rect.rect.x, rect.rect.y}, Vector2{rect.rect.x, rect.rect.w}, Vector2{rect.rect.z, rect.rect.y},
      Vector2{rect.rect.z, rect.rect.y}, Vector2{rect.rect.x, rect.rect.w}, Vector2{rect.rect.z, rect.rect.w},
   };

   auto vertexBuffer = GAPI_CHECK(
      m_device.create_buffer(graphics_api::BufferUsage::VertexBuffer | graphics_api::BufferUsage::TransferDst, sizeof(vertices)));
   GAPI_CHECK_STATUS(vertexBuffer.write_indirect(vertices.data(), sizeof(vertices)));

   struct VertUBO
   {
      Vector2 position;
      Vector2 viewportSize;
   } vsUbo{};
   vsUbo.position = {rect.rect.x, rect.rect.y};
   vsUbo.viewportSize = {1920, 1080};

   auto vsUboBuffer = GAPI_CHECK(
      m_device.create_buffer(graphics_api::BufferUsage::UniformBuffer | graphics_api::BufferUsage::TransferDst, sizeof(VertUBO)));
   GAPI_CHECK_STATUS(vsUboBuffer.write_indirect(&vsUbo, sizeof(VertUBO)));

   struct FragUBO
   {
      Vector4 borderRadius;
      Vector4 backgroundColor;
      Vector2 rectSize;
   } fsUbo{};
   fsUbo.borderRadius = {10, 10, 10, 10};
   fsUbo.backgroundColor = {0, 0, 0, 0.8f};
   fsUbo.rectSize = {rect.rect.z - rect.rect.x, rect.rect.w - rect.rect.y};

   auto fsUboBuffer = GAPI_CHECK(
      m_device.create_buffer(graphics_api::BufferUsage::UniformBuffer | graphics_api::BufferUsage::TransferDst, sizeof(FragUBO)));
   GAPI_CHECK_STATUS(fsUboBuffer.write_indirect(&fsUbo, sizeof(FragUBO)));

   m_rectangles.emplace_back(std::move(vsUboBuffer), std::move(fsUboBuffer), std::move(vertexBuffer));
}

static const auto g_rectangleLayout = render_core::VertexLayout(sizeof(Vector2)).add("position"_name, GAPI_FORMAT(RG, Float32), 0);

void UpdateUserInterfaceJob::render_ui(render_core::BuildContext& ctx)
{
   for (const auto& rectangle : m_rectangles) {
      ctx.bind_vertex_shader("rectangle.vshader"_rc);

      ctx.bind_uniform_buffer(0, &rectangle.vsUbo);

      ctx.bind_fragment_shader("rectangle.fshader"_rc);

      ctx.bind_uniform_buffer(1, &rectangle.fsUbo);

      ctx.bind_vertex_layout(g_rectangleLayout);
      ctx.bind_vertex_buffer(&rectangle.vertexBuffer);

      ctx.draw_primitives(6, 0);
   }

   ctx.bind_vertex_shader("text/render.vshader"_rc);

   ctx.bind_storage_buffer(0, "user_interface.text_draw_call_buffer"_external);

   ctx.bind_fragment_shader("text/render.fshader"_rc);

   ctx.bind_sampled_texture_array(1, m_atlases);

   ctx.push_constant(ctx.screen_size());

   render_core::VertexLayout layout(sizeof(TextVertex));
   layout.add("position"_name, GAPI_FORMAT(RG, Float32), offsetof(TextVertex, position));
   layout.add("uv"_name, GAPI_FORMAT(RG, Float32), offsetof(TextVertex, uv));
   ctx.bind_vertex_layout(layout);

   ctx.bind_vertex_buffer("user_interface.text_vertex_buffer"_external);

   // ctx.draw_primitives(114, 0, 1, 0);
   ctx.draw_indirect_with_count("user_interface.text_draw_call_buffer"_external, "user_interface.text_draw_call_count"_external,
                                g_maxTextDrawCalls, sizeof(TextDrawCall));
}

const TypefaceInfo& UpdateUserInterfaceJob::get_typeface_info(const GlyphProperties& glyphProps)
{
   if (const auto it = m_typefaceInfos.find(glyphProps.hash()); it != m_typefaceInfos.end()) {
      return it->second;
   }

   auto& glyphAtlas = m_glyphCache.find_glyph_atlas(glyphProps);

   const u32 atlasId = m_atlases.size();
   m_atlases.emplace_back(&glyphAtlas.texture());

   const auto transferCmdList = GAPI_CHECK(m_device.create_command_list(graphics_api::WorkType::Transfer));

   GAPI_CHECK_STATUS(transferCmdList.begin(graphics_api::SubmitType::OneTime));
   transferCmdList.copy_buffer(glyphAtlas.storage_buffer(), m_combinedGlyphBuffer, 0, m_glyphOffset, glyphAtlas.storage_buffer().size());
   GAPI_CHECK_STATUS(transferCmdList.finish());

   GAPI_CHECK_STATUS(m_device.submit_command_list_one_time(transferCmdList));

   auto [newIt, ok] =
      m_typefaceInfos.emplace(glyphProps.hash(), TypefaceInfo{atlasId, static_cast<u32>(m_glyphOffset / sizeof(render_core::GlyphInfo))});
   assert(ok);

   m_glyphOffset += glyphAtlas.storage_buffer().size();

   return newIt->second;
}

VertexBufferSection UpdateUserInterfaceJob::allocate_vertex_section(const Name text, const u32 vertexCount)
{
   // TODO: Find more efficient way to do this
   u32 offset = 0;
   u32 unusedSpace = 0;
   for (auto it = m_vertexSections.begin(); it != m_vertexSections.end(); ++it) {
      unusedSpace += it->vertexOffset - offset;
      if (const auto availableSize = it->vertexOffset - offset; vertexCount <= availableSize) {
         const auto dstIt = m_vertexSections.emplace(it, text, offset, vertexCount);
         return *dstIt;
      }
      offset = it->vertexOffset + it->vertexCount;
   }

   assert(offset + vertexCount <= g_maxTextVertices);
   spdlog::info("ui-update-job: allocated vertices: {}", offset + vertexCount);
   spdlog::info("ui-update-job: unused space: {}", unusedSpace);

   m_vertexSections.emplace_back(text, offset, vertexCount);
   return m_vertexSections.back();
}

void UpdateUserInterfaceJob::free_vertex_section(const Name textName)
{
   const auto it = std::ranges::find_if(m_vertexSections, [textName](const auto& sec) { return sec.textName == textName; });
   if (it == m_vertexSections.end()) {
      return;
   }
   m_vertexSections.erase(it);
}

}// namespace triglav::renderer
