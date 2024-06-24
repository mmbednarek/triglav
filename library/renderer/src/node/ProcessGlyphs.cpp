#include "ProcessGlyphs.h"

#include "triglav/graphics_api/PipelineBuilder.h"

namespace triglav::renderer::node {

using namespace name_literals;
namespace gapi = graphics_api;

ProcessGlyphsResources::ProcessGlyphsResources(gapi::Device& device) :
    m_vertexBuffer(GAPI_CHECK(
       device.create_buffer(gapi::BufferUsage::VertexBuffer | gapi::BufferUsage::StorageBuffer | gapi::BufferUsage::TransferDst, sizeof(render_core::GlyphVertex) * 11 * 6)))
{
}

graphics_api::Buffer& ProcessGlyphsResources::vertex_buffer()
{
   return m_vertexBuffer;
}

ProcessGlyphs::ProcessGlyphs(gapi::Device& device, resource::ResourceManager& resourceManager, GlyphCache& glyphCache) :
    m_device(device),
    m_glyphCache(glyphCache),
    m_pipeline(GAPI_CHECK(gapi::ComputePipelineBuilder(device)
                             .compute_shader(resourceManager.get("text.cshader"_rc))
                             .descriptor_binding(gapi::DescriptorType::StorageBuffer)
                             .descriptor_binding(gapi::DescriptorType::StorageBuffer)
                             .descriptor_binding(gapi::DescriptorType::StorageBuffer)
                             .use_push_descriptors(true)
                             .build())),
    m_textBuffer(GAPI_CHECK(device.create_buffer(gapi::BufferUsage::StorageBuffer | gapi::BufferUsage::TransferDst, sizeof(u32) * 11)))
{
   auto encodedStr = font::Charset::European.encode_string("Hello World");
   GAPI_CHECK_STATUS(m_textBuffer.write_indirect(encodedStr.data(), sizeof(u32) * encodedStr.size()));
}

gapi::WorkTypeFlags ProcessGlyphs::work_types() const
{
   return gapi::WorkType::Compute;
}

void ProcessGlyphs::record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& resources,
                                    graphics_api::CommandList& cmdList)
{
   auto& glyphResources = dynamic_cast<ProcessGlyphsResources&>(resources);

   auto& atlas = m_glyphCache.find_glyph_atlas(GlyphProperties{"segoeui/bold.typeface"_rc, 20});

   cmdList.bind_pipeline(m_pipeline);
   cmdList.bind_storage_buffer(0, atlas.storage_buffer());
   cmdList.bind_storage_buffer(1, m_textBuffer);
   cmdList.bind_storage_buffer(2, glyphResources.vertex_buffer());
   cmdList.dispatch(1, 1, 1);
}

std::unique_ptr<render_core::NodeFrameResources> ProcessGlyphs::create_node_resources()
{
   return std::make_unique<ProcessGlyphsResources>(m_device);
}

}// namespace triglav::renderer::node