#include "SpriteRenderer.h"

#include "graphics_api/CommandList.h"
#include "graphics_api/DescriptorWriter.h"
#include "graphics_api/PipelineBuilder.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_transform_2d.hpp>
#undef GLM_ENABLE_EXPERIMENTAL

using namespace triglav::name_literals;

using triglav::Name;
using triglav::ResourceType;
using triglav::render_core::checkResult;
using triglav::render_core::Sprite;
using triglav::render_core::SpriteUBO;
using triglav::resource::ResourceManager;

namespace renderer {

struct Vertex2D
{
   glm::vec3 location;
};

SpriteRenderer::SpriteRenderer(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
                               ResourceManager &resourceManager) :
    m_device(device),
    m_renderPass(renderPass),
    m_resourceManager(resourceManager),
    m_pipeline(checkResult(
            graphics_api::PipelineBuilder(m_device, renderPass)
                    .fragment_shader(resourceManager.get<ResourceType::FragmentShader>("sprite.fshader"_name))
                    .vertex_shader(resourceManager.get<ResourceType::VertexShader>("sprite.vshader"_name))
                    // Descriptor layout
                    .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                        graphics_api::ShaderStage::Vertex)
                    .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                        graphics_api::ShaderStage::Fragment)
                    .vertex_topology(graphics_api::VertexTopology::TriangleStrip)
                    .enable_depth_test(false)
                    .build())),
    m_descriptorPool(checkResult(m_pipeline.create_descriptor_pool(40, 40, 40))),
    m_sampler(checkResult(device.create_sampler(true)))
{
}

Sprite SpriteRenderer::create_sprite(const Name textureName)
{
   const auto &texture = m_resourceManager.get<ResourceType::Texture>(textureName);
   return this->create_sprite_from_texture(texture);
}

Sprite SpriteRenderer::create_sprite_from_texture(const graphics_api::Texture &texture)
{
   graphics_api::UniformBuffer<SpriteUBO> ubo(m_device);

   auto descriptors = checkResult(m_descriptorPool.allocate_array(1));

   {
      graphics_api::DescriptorWriter writer(m_device, descriptors[0]);
      writer.set_uniform_buffer(0, ubo);
      writer.set_sampled_texture(1, texture, m_sampler);
   }

   return Sprite{static_cast<float>(texture.width()), static_cast<float>(texture.height()), std::move(ubo),
                 std::move(descriptors)};
}

void SpriteRenderer::update_resolution(const graphics_api::Resolution &resolution)
{
   m_resolution = resolution;
}

void SpriteRenderer::set_active_command_list(graphics_api::CommandList *commandList)
{
   m_commandList = commandList;
}

void SpriteRenderer::begin_render() const
{
   assert(m_commandList);
   m_commandList->bind_pipeline(m_pipeline);
}

void SpriteRenderer::draw_sprite(const Sprite &sprite, const glm::vec2 position, const glm::vec2 scale) const
{
   assert(m_commandList != nullptr);

   const auto [viewportWidth, viewportHeight] = m_resolution;

   const auto scaleX = 2.0f * scale.x * sprite.width / static_cast<float>(viewportWidth);
   const auto scaleY = 2.0f * scale.y * sprite.height / static_cast<float>(viewportHeight);
   const auto transX = (position.x - 0.5f * static_cast<float>(viewportWidth)) / scale.x / sprite.width;
   const auto transY = (position.y - 0.5f * static_cast<float>(viewportHeight)) / scale.y / sprite.height;

   const auto scaleMat   = glm::scale(glm::mat3(1), glm::vec2(scaleX, scaleY));
   sprite.ubo->transform = glm::translate(scaleMat, glm::vec2(transX, transY));

   m_commandList->bind_descriptor_set(sprite.descriptors[0]);
   m_commandList->draw_primitives(4, 0);
}

}// namespace renderer