#include "ProcessGlyphs.h"

#include "triglav/graphics_api/PipelineBuilder.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_transform_2d.hpp>
#undef GLM_ENABLE_EXPERIMENTAL

namespace triglav::renderer::node {

using namespace name_literals;
namespace gapi = graphics_api;

constexpr auto g_charactersInStagingBuffer = 2048;

struct TextColorConstant
{
   glm::vec3 color;
};

u32 next_multiple_of(u32 value, u32 offset)
{
   if (offset <= 1) {
      return value;
   }
   return value + offset - (value % offset);
}

TextResources::TextResources(graphics_api::Device& device, const render_core::GlyphAtlas* atlas, const ui_core::Text* textInfo,
                             const u32 vertexCount) :
    atlas(atlas),
    textInfo(textInfo),
    vertexBuffer(
       GAPI_CHECK(device.create_buffer(gapi::BufferUsage::VertexBuffer | gapi::BufferUsage::StorageBuffer | gapi::BufferUsage::TransferDst,
                                       sizeof(render_core::GlyphVertex) * vertexCount))),
    ubo(device),
    vertexCount(vertexCount)
{
}

void TextResources::update_size(graphics_api::Device& device, const u32 newVertexCount)
{
   this->vertexCount = newVertexCount;

   const auto newBufferSize = sizeof(render_core::GlyphVertex) * newVertexCount;
   if (newBufferSize <= this->vertexBuffer.size())
      return;

   this->vertexBuffer = GAPI_CHECK(device.create_buffer(
      gapi::BufferUsage::VertexBuffer | gapi::BufferUsage::StorageBuffer | gapi::BufferUsage::TransferDst, newBufferSize));
}

ProcessGlyphsResources::ProcessGlyphsResources(gapi::Device& device, gapi::Pipeline& pipeline, GlyphCache& glyphCache,
                                               ui_core::Viewport& viewport) :
    m_device(device),
    m_pipeline(pipeline),
    m_glyphCache(glyphCache),
    m_textStagingBuffer(
       GAPI_CHECK(device.create_buffer(gapi::BufferUsage::HostVisible | gapi::BufferUsage::StorageBuffer | gapi::BufferUsage::TransferSrc,
                                       sizeof(u32) * g_charactersInStagingBuffer))),
    m_textBuffer(GAPI_CHECK(
       device.create_buffer(gapi::BufferUsage::StorageBuffer | gapi::BufferUsage::TransferDst, sizeof(u32) * g_charactersInStagingBuffer))),
    m_onAddedTextSink(viewport.OnAddedText.connect<&ProcessGlyphsResources::on_added_text>(this)),
    m_onTextChangeContentSink(viewport.OnTextChangeContent.connect<&ProcessGlyphsResources::on_added_text>(this))
{
}

void ProcessGlyphsResources::on_added_text(Name name, const ui_core::Text& text)
{
   m_pendingUpdates.emplace_back(name, &text, 0, 0);
}

void ProcessGlyphsResources::update_resources(graphics_api::CommandList& cmdList)
{
   if (m_pendingUpdates.empty())
      return;

   u32 textOffset = 0;

   {
      auto mapping = GAPI_CHECK(m_textStagingBuffer.map_memory());
      auto* bufferPtr = static_cast<u32*>(*mapping);
      for (auto& update : m_pendingUpdates) {
         const auto textCount = font::Charset::European.encode_string_to(update.textInfo->content, bufferPtr + textOffset);
         update.textOffset = textOffset;
         update.textCount = textCount;
         textOffset = next_multiple_of(textOffset + textCount, m_device.min_storage_buffer_alignment() / sizeof(u32));
      }
   }

   cmdList.copy_buffer(m_textStagingBuffer, m_textBuffer, 0, 0, textOffset * sizeof(u32));

   cmdList.execution_barrier(gapi::PipelineStage::Transfer, gapi::PipelineStage::ComputeShader);

   cmdList.bind_pipeline(m_pipeline);

   for (const auto& update : m_pendingUpdates) {
      auto& atlas = m_glyphCache.find_glyph_atlas(GlyphProperties{update.textInfo->typefaceName, update.textInfo->fontSize});

      const auto vertexCount = 6 * update.textCount;

      auto it = m_textResources.find(update.name);
      if (it == m_textResources.end()) {
         bool ok;
         std::tie(it, ok) = m_textResources.emplace(update.name, TextResources{m_device, &atlas, update.textInfo, vertexCount});
         assert(ok);
      } else {
         it->second.update_size(m_device, vertexCount);
      }

      cmdList.bind_storage_buffer(0, atlas.storage_buffer());
      cmdList.bind_storage_buffer(1, m_textBuffer, sizeof(u32) * update.textOffset, sizeof(u32) * update.textCount);
      cmdList.bind_storage_buffer(2, it->second.vertexBuffer);
      cmdList.dispatch(1, 1, 1);
   }

   m_pendingUpdates.clear();
}

void ProcessGlyphsResources::draw(graphics_api::CommandList& cmdList, const graphics_api::Pipeline& textPipeline,
                                  const glm::vec2& viewportSize)
{
   cmdList.bind_pipeline(textPipeline);

   for (const auto& resources : std::views::values(m_textResources)) {
      ProcessGlyphsResources::draw_text(cmdList, resources, viewportSize);
   }
}

void ProcessGlyphsResources::draw_text(graphics_api::CommandList& cmdList, const TextResources& resources, const glm::vec2& viewportSize)
{
   TextColorConstant constant{resources.textInfo->color};
   cmdList.push_constant(graphics_api::PipelineStage::FragmentShader, constant);

   const auto sc = glm::scale(glm::mat3(1), glm::vec2(2.0f / viewportSize.x, 2.0f / viewportSize.y));
   resources.ubo->transform = glm::translate(
      sc, glm::vec2(resources.textInfo->position.x - viewportSize.x / 2.0f, resources.textInfo->position.y - viewportSize.y / 2.0f));

   cmdList.bind_vertex_buffer(resources.vertexBuffer, 0);
   cmdList.bind_uniform_buffer(0, resources.ubo);
   cmdList.bind_texture(1, resources.atlas->texture());
   cmdList.draw_primitives(static_cast<int>(resources.vertexCount), 0);
}

ProcessGlyphs::ProcessGlyphs(gapi::Device& device, resource::ResourceManager& resourceManager, GlyphCache& glyphCache,
                             ui_core::Viewport& viewport) :
    m_device(device),
    m_glyphCache(glyphCache),
    m_viewport(viewport),
    m_pipeline(GAPI_CHECK(gapi::ComputePipelineBuilder(device)
                             .compute_shader(resourceManager.get("text.cshader"_rc))
                             .descriptor_binding(gapi::DescriptorType::StorageBuffer)
                             .descriptor_binding(gapi::DescriptorType::StorageBuffer)
                             .descriptor_binding(gapi::DescriptorType::StorageBuffer)
                             .use_push_descriptors(true)
                             .build()))
{
}

gapi::WorkTypeFlags ProcessGlyphs::work_types() const
{
   return gapi::WorkType::Compute;
}

void ProcessGlyphs::record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& resources,
                                    graphics_api::CommandList& cmdList)
{
   auto& glyphResources = dynamic_cast<ProcessGlyphsResources&>(resources);
   glyphResources.update_resources(cmdList);
}

std::unique_ptr<render_core::NodeFrameResources> ProcessGlyphs::create_node_resources()
{
   return std::make_unique<ProcessGlyphsResources>(m_device, m_pipeline, m_glyphCache, m_viewport);
}

}// namespace triglav::renderer::node