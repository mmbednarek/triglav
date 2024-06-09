#include "TextRenderer.h"

#include "triglav/graphics_api/PipelineBuilder.h"

#include <glm/vec3.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_transform_2d.hpp>
#undef GLM_ENABLE_EXPERIMENTAL

namespace triglav::renderer {

using namespace name_literals;

struct TextColorConstant
{
   glm::vec3 color;
};

TextRenderer::TextRenderer(graphics_api::Device& device, resource::ResourceManager& resourceManager,
                           graphics_api::RenderTarget& renderTarget) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_pipeline(GAPI_CHECK(graphics_api::GraphicsPipelineBuilder(device, renderTarget)
                             .fragment_shader(m_resourceManager.get("text.fshader"_rc))
                             .vertex_shader(m_resourceManager.get("text.vshader"_rc))
                             // Vertex description
                             .begin_vertex_layout<render_core::GlyphVertex>()
                             .vertex_attribute(GAPI_FORMAT(RG, Float32), offsetof(render_core::GlyphVertex, position))
                             .vertex_attribute(GAPI_FORMAT(RG, Float32), offsetof(render_core::GlyphVertex, texCoord))
                             .end_vertex_layout()
                             // Descriptor layout
                             .descriptor_binding(graphics_api::DescriptorType::UniformBuffer, graphics_api::PipelineStage::VertexShader)
                             .descriptor_binding(graphics_api::DescriptorType::ImageSampler, graphics_api::PipelineStage::FragmentShader)
                             .push_constant(graphics_api::PipelineStage::FragmentShader, sizeof(TextColorConstant))
                             .enable_depth_test(false)
                             .enable_blending(true)
                             .use_push_descriptors(true)
                             .vertex_topology(graphics_api::VertexTopology::TriangleList)
                             .build()))
{
}

void TextRenderer::bind_pipeline(graphics_api::CommandList& cmdList)
{
   cmdList.bind_pipeline(m_pipeline);
}

TextObject TextRenderer::create_text_object(GlyphAtlasName atlasName, std::string_view content)
{
   graphics_api::UniformBuffer<render_core::SpriteUBO> ubo(m_device);

   auto& atlas = m_resourceManager.get(atlasName);

   render_core::TextMetric metric{};
   const auto vertices = atlas.create_glyph_vertices(content, &metric);
   graphics_api::VertexArray<render_core::GlyphVertex> gpuVertices(m_device, vertices.size());
   gpuVertices.write(vertices.data(), vertices.size());

   return TextObject(atlasName, metric, std::move(ubo), std::move(gpuVertices), vertices.size());
}

void TextRenderer::draw_text(graphics_api::CommandList& cmdList, const TextObject& textObject, const glm::vec2& viewportSize,
                             const glm::vec2& position, const glm::vec4& color) const
{
   TextColorConstant constant{color};
   cmdList.push_constant(graphics_api::PipelineStage::FragmentShader, constant);

   const auto sc = glm::scale(glm::mat3(1), glm::vec2(2.0f / viewportSize.x, 2.0f / viewportSize.y));
   textObject.ubo->transform = glm::translate(sc, glm::vec2(position.x - viewportSize.x / 2.0f, position.y - viewportSize.y / 2.0f));

   auto& atlas = m_resourceManager.get(textObject.glyphAtlas);

   cmdList.bind_vertex_array(textObject.vertices);
   cmdList.bind_uniform_buffer(0, textObject.ubo);
   cmdList.bind_texture(1, atlas.texture());
   cmdList.draw_primitives(static_cast<int>(textObject.vertexCount), 0);
}

void TextRenderer::update_text(TextObject& textObject, std::string_view content)
{
   auto& atlas = m_resourceManager.get(textObject.glyphAtlas);

   const auto vertices = atlas.create_glyph_vertices(content, &textObject.metric);
   if (vertices.size() > textObject.vertices.count()) {
      graphics_api::VertexArray<render_core::GlyphVertex> gpuVertices(m_device, vertices.size());
      gpuVertices.write(vertices.data(), vertices.size());
      textObject.vertexCount = vertices.size();
      textObject.vertices = std::move(gpuVertices);
   } else {
      textObject.vertices.write(vertices.data(), vertices.size());
      textObject.vertexCount = vertices.size();
   }
}

}// namespace triglav::renderer
