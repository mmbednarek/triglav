#include "ShadingRenderer.h"

#include <cstring>
#include <random>

#include "triglav/graphics_api/CommandList.h"
#include "triglav/graphics_api/DescriptorWriter.h"
#include "triglav/graphics_api/PipelineBuilder.h"
#include "triglav/render_core/RenderCore.hpp"

using namespace triglav::name_literals;
using triglav::resource::ResourceManager;
using triglav::ResourceType;
using triglav::render_core::checkResult;

namespace triglav::renderer {

ShadingRenderer::ShadingRenderer(
        graphics_api::Device &device, graphics_api::RenderPass &renderPass,
        ResourceManager &resourceManager, const graphics_api::Texture &colorTexture,
        const graphics_api::Texture &positionTexture, const graphics_api::Texture &normalTexture,
        const graphics_api::Texture &aoTexture, const graphics_api::Texture &shadowMapTexture) :
    m_device(device),
    m_pipeline(checkResult(graphics_api::PipelineBuilder(m_device, renderPass)
                                   .fragment_shader(resourceManager.get<ResourceType::FragmentShader>("shading.fshader"_name))
                                   .vertex_shader(resourceManager.get<ResourceType::VertexShader>("shading.vshader"_name))
                                   // Vertex description
                                   .begin_vertex_layout<geometry::Vertex>()
                                   .end_vertex_layout()
                                   // Descriptor layout
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::PipelineStage::FragmentShader)
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::PipelineStage::FragmentShader)
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::PipelineStage::FragmentShader)
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::PipelineStage::FragmentShader)
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::PipelineStage::FragmentShader)
                                   .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                                       graphics_api::PipelineStage::FragmentShader)
                                   .push_constant(graphics_api::PipelineStage::FragmentShader, sizeof(PushConstant))
                                   .enable_depth_test(false)
                                   .vertex_topology(graphics_api::VertexTopology::TriangleStrip)
                                   .build())),
    m_descriptorPool(checkResult(m_pipeline.create_descriptor_pool(1, 5, 1))),
    m_sampler(resourceManager.get<ResourceType::Sampler>("linear_repeat_mlod0.sampler"_name)),
    m_descriptors(checkResult(m_descriptorPool.allocate_array(1))),
    m_uniformBuffer(m_device)
{
   graphics_api::DescriptorWriter writer(m_device, m_descriptors[0]);
   writer.set_sampled_texture(0, colorTexture, m_sampler);
   writer.set_sampled_texture(1, positionTexture, m_sampler);
   writer.set_sampled_texture(2, normalTexture, m_sampler);
   writer.set_sampled_texture(3, aoTexture, m_sampler);
   writer.set_sampled_texture(4, shadowMapTexture, m_sampler);
   writer.set_uniform_buffer(5, m_uniformBuffer);
}

void ShadingRenderer::update_textures(const graphics_api::Texture &colorTexture,
                                             const graphics_api::Texture &positionTexture,
                                             const graphics_api::Texture &normalTexture,
                                             const graphics_api::Texture &aoTexture,
                                             const graphics_api::Texture &shadowMapTexture) const
{
   graphics_api::DescriptorWriter writer(m_device, m_descriptors[0]);
   writer.set_sampled_texture(0, colorTexture, m_sampler);
   writer.set_sampled_texture(1, positionTexture, m_sampler);
   writer.set_sampled_texture(2, normalTexture, m_sampler);
   writer.set_sampled_texture(3, aoTexture, m_sampler);
   writer.set_sampled_texture(4, shadowMapTexture, m_sampler);
   writer.set_uniform_buffer(5, m_uniformBuffer);
}

void ShadingRenderer::draw(graphics_api::CommandList &cmdList, const glm::vec3 &lightPosition,
                                  const glm::mat4 &shadowMapMat, const bool ssaoEnabled) const
{
   m_uniformBuffer->shadowMapMat = shadowMapMat;

   cmdList.bind_pipeline(m_pipeline);
   cmdList.bind_descriptor_set(m_descriptors[0]);

   PushConstant pushConstant{
           .lightPosition = lightPosition,
           .enableSSAO    = ssaoEnabled,
   };
   cmdList.push_constant(graphics_api::PipelineStage::FragmentShader, pushConstant);

   cmdList.draw_primitives(4, 0);
}

}// namespace renderer