#include "SpriteRenderer.h"

#include "triglav/graphics_api/CommandList.h"
#include "triglav/graphics_api/DescriptorWriter.h"
#include "triglav/graphics_api/PipelineBuilder.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_transform_2d.hpp>
#undef GLM_ENABLE_EXPERIMENTAL

using namespace triglav::name_literals;

using triglav::ResourceName;
using triglav::ResourceType;
using triglav::render_core::checkResult;
using triglav::render_core::Sprite;
using triglav::render_core::SpriteUBO;
using triglav::resource::ResourceManager;

namespace triglav::renderer {

struct Vertex2D
{
   glm::vec3 location;
};

SpriteRenderer::SpriteRenderer(graphics_api::Device& device, graphics_api::RenderTarget& renderTarget, ResourceManager& resourceManager) :
    m_device(device),
    m_renderTarget(renderTarget),
    m_resourceManager(resourceManager),
    m_pipeline(checkResult(graphics_api::GraphicsPipelineBuilder(m_device, renderTarget)
                              .fragment_shader(resourceManager.get("sprite.fshader"_rc))
                              .vertex_shader(resourceManager.get("sprite.vshader"_rc))
                              // Descriptor layout
                              .descriptor_binding(graphics_api::DescriptorType::UniformBuffer, graphics_api::PipelineStage::VertexShader)
                              .descriptor_binding(graphics_api::DescriptorType::ImageSampler, graphics_api::PipelineStage::FragmentShader)
                              .vertex_topology(graphics_api::VertexTopology::TriangleStrip)
                              .enable_depth_test(false)
                              .use_push_descriptors(true)
                              .build())),
    m_descriptorPool(checkResult(m_pipeline.create_descriptor_pool(40, 40, 40)))
{
}

Sprite SpriteRenderer::create_sprite(const ResourceName textureName)
{
   const auto& texture = m_resourceManager.get<ResourceType::Texture>(textureName);
   return this->create_sprite_from_texture(texture);
}

Sprite SpriteRenderer::create_sprite_from_texture(const graphics_api::Texture& texture)
{
   graphics_api::UniformBuffer<SpriteUBO> ubo(m_device);
   return Sprite{&texture, std::move(ubo)};
}

void SpriteRenderer::update_resolution(const graphics_api::Resolution& resolution)
{
   m_resolution = resolution;
}

void SpriteRenderer::begin_render(graphics_api::CommandList& cmdList) const
{
   cmdList.bind_pipeline(m_pipeline);
}

void SpriteRenderer::draw_sprite(graphics_api::CommandList& cmdList, const Sprite& sprite, const glm::vec2 position,
                                 const glm::vec2 scale) const
{
   const auto [viewportWidth, viewportHeight] = m_resolution;

   const auto scaleX = 2.0f * scale.x * sprite.texture->width() / static_cast<float>(viewportWidth);
   const auto scaleY = 2.0f * scale.y * sprite.texture->height() / static_cast<float>(viewportHeight);
   const auto transX = (position.x - 0.5f * static_cast<float>(viewportWidth)) / scale.x / sprite.texture->width();
   const auto transY = (position.y - 0.5f * static_cast<float>(viewportHeight)) / scale.y / sprite.texture->height();

   const auto scaleMat = glm::scale(glm::mat3(1), glm::vec2(scaleX, scaleY));
   sprite.ubo->transform = glm::translate(scaleMat, glm::vec2(transX, transY));

   cmdList.bind_uniform_buffer(0, sprite.ubo);
   cmdList.bind_texture(1, *sprite.texture);
   cmdList.draw_primitives(4, 0);
}

}// namespace triglav::renderer