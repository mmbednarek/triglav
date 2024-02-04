#include "PostProcessingRenderer.h"

#include "graphics_api/CommandList.h"
#include "graphics_api/DescriptorWriter.h"
#include "graphics_api/PipelineBuilder.h"

namespace renderer {

PostProcessingRenderer::PostProcessingRenderer(graphics_api::Device &device,
                                               graphics_api::RenderPass &renderPass,
                                               const ResourceManager &resourceManager,
                                               const graphics_api::Texture &colorTexture) :
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
                                   .enable_depth_test(false)
                                   .vertex_topology(graphics_api::VertexTopology::TriangleStrip)
                                   .push_constant(graphics_api::ShaderStage::Fragment, sizeof(PushConstants))
                                   .build())),
    m_descriptorPool(checkResult(m_pipeline.create_descriptor_pool(1, 1, 1))),
    m_sampler(checkResult(device.create_sampler(false))),
    m_descriptors(checkResult(m_descriptorPool.allocate_array(1)))
{
   graphics_api::DescriptorWriter writer(m_device, m_descriptors[0]);
   writer.set_sampled_texture(0, colorTexture, m_sampler);
}

void PostProcessingRenderer::update_texture(const graphics_api::Texture &colorTexture) const
{
   graphics_api::DescriptorWriter writer(m_device, m_descriptors[0]);
   writer.set_sampled_texture(0, colorTexture, m_sampler);
}

void PostProcessingRenderer::draw(graphics_api::CommandList &cmdList, const bool enableFXAA) const
{
   cmdList.bind_pipeline(m_pipeline);
   cmdList.bind_descriptor_set(m_descriptors[0]);
   PushConstants constants{
           .enableFXAA = enableFXAA,
   };
   cmdList.push_constant(graphics_api::ShaderStage::Fragment, constants);
   cmdList.draw_primitives(4, 0);
}

}// namespace renderer