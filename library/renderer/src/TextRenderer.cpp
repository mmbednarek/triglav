#include "TextRenderer.h"

#include "graphics_api/CommandList.h"
#include "graphics_api/DescriptorWriter.h"
#include "graphics_api/PipelineBuilder.h"

#include "GlyphAtlas.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_transform_2d.hpp>
#undef GLM_ENABLE_EXPERIMENTAL

namespace renderer {

TextRenderer::TextRenderer(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
                           ResourceManager &resourceManager) :
    m_device(device),
    m_renderPass(renderPass),
    m_resourceManager(resourceManager),
    m_pipeline(
            checkResult(graphics_api::PipelineBuilder(m_device, renderPass)
                                .fragment_shader(resourceManager.shader("fsh:text"_name))
                                .vertex_shader(resourceManager.shader("vsh:text"_name))
                                // Vertex description
                                .begin_vertex_layout<GlyphVertex>()
                                .vertex_attribute(GAPI_FORMAT(RG, Float32), offsetof(GlyphVertex, position))
                                .vertex_attribute(GAPI_FORMAT(RG, Float32), offsetof(GlyphVertex, texCoord))
                                .end_vertex_layout()
                                // Descriptor layout
                                .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                                    graphics_api::ShaderStage::Vertex)
                                .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                    graphics_api::ShaderStage::Fragment)
                                .push_constant(graphics_api::ShaderStage::Fragment, sizeof(TextColorConstant))
                                .enable_depth_test(false)
                                .vertex_topology(graphics_api::VertexTopology::TriangleList)
                                .build())),
    m_descriptorPool(checkResult(m_pipeline.create_descriptor_pool(20, 20, 20))),
    m_sampler(checkResult(m_device.create_sampler(false)))
{
}

void TextRenderer::begin_render(graphics_api::CommandList &cmdList) const
{
   cmdList.bind_pipeline(m_pipeline);
}

TextObject TextRenderer::create_text_object(const GlyphAtlas &atlas, const std::string_view text)
{
   auto descriptors = checkResult(m_descriptorPool.allocate_array(1));

   graphics_api::UniformBuffer<SpriteUBO> ubo(m_device);

   graphics_api::DescriptorWriter descWriter(m_device, descriptors[0]);
   descWriter.set_uniform_buffer(0, ubo);
   descWriter.set_sampled_texture(1, atlas.texture(), m_sampler);

   TextMetric metric{};
   const auto vertices = atlas.create_glyph_vertices(text, &metric);
   graphics_api::VertexArray<GlyphVertex> gpuVertices(m_device, vertices.size());
   gpuVertices.write(vertices.data(), vertices.size());

   return TextObject(std::string{text}, metric, std::move(descriptors), std::move(ubo),
                     std::move(gpuVertices), vertices.size());
}

void TextRenderer::update_text_object(const GlyphAtlas &atlas, TextObject &object,
                                      const std::string_view text) const
{
   if (object.text == text)
      return;

   const auto vertices = atlas.create_glyph_vertices(text, &object.metric);
   if (vertices.size() > object.vertices.count()) {
      graphics_api::VertexArray<GlyphVertex> gpuVertices(m_device, vertices.size());
      gpuVertices.write(vertices.data(), vertices.size());
      object.vertexCount = vertices.size();
      object.vertices    = std::move(gpuVertices);
   } else {
      object.vertices.write(vertices.data(), vertices.size());
      object.vertexCount = vertices.size();
   }

   object.text = text;
}

void TextRenderer::update_resolution(const graphics_api::Resolution &resolution)
{
   m_resolution = resolution;
}

void TextRenderer::draw_text_object(const graphics_api::CommandList &cmdList, const TextObject &textObj,
                                    const glm::vec2 position, const glm::vec3 color) const
{
   const auto [viewportWidth, viewportHeight] = m_resolution;

   TextColorConstant constant{color};
   cmdList.push_constant(graphics_api::ShaderStage::Fragment, constant);

   // const auto transX =
   //         (position.x / static_cast<float>(viewportWidth) - 0.5f * static_cast<float>(viewportWidth));
   // const auto transY =
   //         (position.y / static_cast<float>(viewportHeight) - 0.5f * static_cast<float>(viewportHeight));

   // textObj.ubo->transform = glm::translate(glm::mat3(1), glm::vec2(transX, transY));
   const auto sc = glm::scale(glm::mat3(1), glm::vec2(2.0f / static_cast<float>(viewportWidth),
                                                      2.0f / static_cast<float>(viewportHeight)));
   textObj.ubo->transform =
           glm::translate(sc, glm::vec2(position.x - static_cast<float>(viewportWidth) / 2.0f,
                                        position.y - static_cast<float>(viewportHeight) / 2.0f));


   cmdList.bind_vertex_array(textObj.vertices);
   cmdList.bind_descriptor_set(textObj.descriptors[0]);
   cmdList.draw_primitives(textObj.vertexCount, 0);
}

}// namespace renderer