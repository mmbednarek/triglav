#include "PostProcessingRenderer.h"

#include "graphics_api/CommandList.h"
#include "graphics_api/DescriptorWriter.h"
#include "graphics_api/PipelineBuilder.h"

namespace renderer {

PostProcessingRenderer::PostProcessingRenderer(
        graphics_api::Device &device, graphics_api::RenderPass &renderPass,
        const ResourceManager &resourceManager, const graphics_api::Texture &colorTexture,
        const graphics_api::Texture &positionTexture, const graphics_api::Texture &normalTexture,
        const graphics_api::Texture &depthTexture, const graphics_api::Texture &noiseTexture,
        const graphics_api::Texture &shadowMapTexture) :
    m_device(device),
    m_pipeline(checkResult(graphics_api::PipelineBuilder(m_device, renderPass)
                                   .fragment_shader(resourceManager.shader("fsh:post_processing"_name))
                                   .vertex_shader(resourceManager.shader("vsh:post_processing"_name))
                                   // Vertex description
                                   .begin_vertex_layout<geometry::Vertex>()
                                   .end_vertex_layout()
                                   // Descriptor layout
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::ShaderStage::Fragment)
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::ShaderStage::Fragment)
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::ShaderStage::Fragment)
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::ShaderStage::Fragment)
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::ShaderStage::Fragment)
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::ShaderStage::Fragment)
                                   .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                                       graphics_api::ShaderStage::Fragment)
                                   .push_constant(graphics_api::ShaderStage::Fragment, sizeof(PushConstant))
                                   .enable_depth_test(false)
                                   .vertex_topology(graphics_api::VertexTopology::TriangleStrip)
                                   .build())),
    m_descriptorPool(checkResult(m_pipeline.create_descriptor_pool(1, 6, 1))),
    m_sampler(checkResult(device.create_sampler(false))),
    m_descriptors(checkResult(m_descriptorPool.allocate_array(1))),
    m_uniformBuffer(m_device)
{
   graphics_api::DescriptorWriter writer(m_device, m_descriptors[0]);
   writer.set_sampled_texture(0, colorTexture, m_sampler);
   writer.set_sampled_texture(1, positionTexture, m_sampler);
   writer.set_sampled_texture(2, normalTexture, m_sampler);
   writer.set_sampled_texture(3, depthTexture, m_sampler);
   writer.set_sampled_texture(4, noiseTexture, m_sampler);
   writer.set_sampled_texture(5, shadowMapTexture, m_sampler);
   writer.set_uniform_buffer(6, m_uniformBuffer);
}

void PostProcessingRenderer::draw(graphics_api::CommandList &cmdList, const glm::vec3 &lightPosition,
                                  const glm::vec3 &cameraPosition, const glm::mat4 &shadowMapViewProj) const
{
   m_uniformBuffer->shadowMapViewProj = shadowMapViewProj;

   cmdList.bind_pipeline(m_pipeline);
   cmdList.bind_descriptor_set(m_descriptors[0]);

   PushConstant pushConstant{
           .lightPosition  = lightPosition,
           .cameraPosition = cameraPosition,
   };
   cmdList.push_constant(graphics_api::ShaderStage::Fragment, pushConstant);

   cmdList.draw_primitives(4, 0);
}

}// namespace renderer