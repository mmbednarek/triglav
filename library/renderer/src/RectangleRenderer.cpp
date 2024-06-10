#include "RectangleRenderer.h"

#include "triglav/graphics_api/CommandList.h"
#include "triglav/graphics_api/DescriptorWriter.h"
#include "triglav/graphics_api/PipelineBuilder.h"

using namespace triglav::name_literals;
using triglav::ResourceType;
using triglav::render_core::checkResult;
using triglav::resource::ResourceManager;

namespace triglav::renderer {

RectangleRenderer::RectangleRenderer(graphics_api::Device& device, graphics_api::RenderTarget& renderTarget,
                                     ResourceManager& resourceManager) :
    m_device(device),
    m_pipeline(checkResult(graphics_api::GraphicsPipelineBuilder(device, renderTarget)
                              .fragment_shader(resourceManager.get("rectangle.fshader"_rc))
                              .vertex_shader(resourceManager.get("rectangle.vshader"_rc))
                              .begin_vertex_layout<glm::vec2>()
                              .vertex_attribute(GAPI_FORMAT(RG, Float32), 0)
                              .end_vertex_layout()
                              .descriptor_binding(graphics_api::DescriptorType::UniformBuffer, graphics_api::PipelineStage::VertexShader)
                              .descriptor_binding(graphics_api::DescriptorType::UniformBuffer, graphics_api::PipelineStage::FragmentShader)
                              .enable_depth_test(false)
                              .enable_blending(true)
                              .build())),
    m_descriptorPool(checkResult(m_pipeline.create_descriptor_pool(20, 1, 20)))
{
}

Rectangle RectangleRenderer::create_rectangle(const glm::vec4 rect, glm::vec4 backgroundColor)
{
   // rect: {left, top, right, bottom}
   std::array vertices{
      glm::vec2{rect.x, rect.y}, glm::vec2{rect.x, rect.w}, glm::vec2{rect.z, rect.y},
      glm::vec2{rect.z, rect.y}, glm::vec2{rect.x, rect.w}, glm::vec2{rect.z, rect.w},
   };
   graphics_api::VertexArray<glm::vec2> array(m_device, vertices.size());
   array.write(vertices.data(), vertices.size());

   graphics_api::UniformBuffer<Rectangle::VertexUBO> vertexUbo{m_device};
   *vertexUbo = Rectangle::VertexUBO{};
   graphics_api::UniformBuffer<Rectangle::FragmentUBO> fragmentUbo{m_device};
   *fragmentUbo = Rectangle::FragmentUBO{
      .backgroundColor = backgroundColor,
   };

   auto descriptors = checkResult(m_descriptorPool.allocate_array(1));

   graphics_api::DescriptorWriter writer(m_device, descriptors[0]);
   writer.set_uniform_buffer(0, vertexUbo);
   writer.set_uniform_buffer(1, fragmentUbo);

   return Rectangle{rect, std::move(array), std::move(vertexUbo), std::move(fragmentUbo), std::move(descriptors)};
}

void RectangleRenderer::update_rectangle(Rectangle& rectangle, glm::vec4 rectCoords, glm::vec4 backgroundColor)
{
   std::array vertices{
      glm::vec2{rectCoords.x, rectCoords.y}, glm::vec2{rectCoords.x, rectCoords.w}, glm::vec2{rectCoords.z, rectCoords.y},
      glm::vec2{rectCoords.z, rectCoords.y}, glm::vec2{rectCoords.x, rectCoords.w}, glm::vec2{rectCoords.z, rectCoords.w},
   };
   graphics_api::VertexArray<glm::vec2> array(m_device, vertices.size());
   array.write(vertices.data(), vertices.size());

   rectangle.rect = rectCoords;
   rectangle.array = std::move(array);
   rectangle.fragmentUBO->backgroundColor = backgroundColor;
}

void RectangleRenderer::begin_render(graphics_api::CommandList& cmdList) const
{
   cmdList.bind_pipeline(m_pipeline);
}

void RectangleRenderer::draw(graphics_api::CommandList& cmdList, const Rectangle& rect, const graphics_api::Resolution& resolution) const
{
   rect.vertexUBO->viewportSize = {resolution.width, resolution.height};
   rect.vertexUBO->position = {rect.rect.x, rect.rect.y};
   rect.fragmentUBO->rectSize = {rect.rect.z - rect.rect.x, rect.rect.w - rect.rect.y};

   cmdList.bind_descriptor_set(rect.descriptors[0]);
   cmdList.bind_vertex_array(rect.array);
   cmdList.draw_primitives(static_cast<int>(rect.array.count()), 0);
}

}// namespace triglav::renderer