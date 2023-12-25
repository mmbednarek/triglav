#include "Context2D.h"

#include "graphics_api/PipelineBuilder.h"

namespace renderer {

Context2D::Context2D(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
                     ResourceManager &resourceManager) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_pipeline(checkResult(
            graphics_api::PipelineBuilder(m_device, renderPass)
                    .fragment_shader(resourceManager.shader("fsh:sprite"_name))
                    .vertex_shader(resourceManager.shader("vsh:sprite"_name))
                    // Descriptor layout
                    .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                        graphics_api::ShaderStage::Vertex)
                    .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                        graphics_api::ShaderStage::Fragment)
                    .enable_depth_test(false)
                    .new_build())),
    m_descriptorPool(checkResult(m_pipeline.create_descriptor_pool(100, 100, 100))),
    m_sampler(checkResult(device.create_sampler(true)))
{
}

}// namespace renderer