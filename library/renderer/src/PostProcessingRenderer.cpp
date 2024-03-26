#include "PostProcessingRenderer.h"

#include "triglav/graphics_api/CommandList.h"
#include "triglav/graphics_api/DescriptorWriter.h"
#include "triglav/graphics_api/PipelineBuilder.h"
#include "triglav/render_core/RenderCore.hpp"

using namespace triglav::name_literals;
using triglav::render_core::checkResult;
using triglav::resource::ResourceManager;
using triglav::ResourceType;

namespace triglav::renderer {

PostProcessingRenderer::PostProcessingRenderer(graphics_api::Device &device,
                                               graphics_api::RenderTarget &renderPass,
                                               ResourceManager &resourceManager,
                                               const graphics_api::Texture &colorTexture,
                                               const graphics_api::Texture &overlayTexture) :
    m_device(device),
    m_pipeline(checkResult(graphics_api::PipelineBuilder(m_device, renderPass)
                                   .fragment_shader(resourceManager.get<ResourceType::FragmentShader>("post_processing.fshader"_name))
                                   .vertex_shader(resourceManager.get<ResourceType::VertexShader>("post_processing.vshader"_name))
                                   // Descriptor layout
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::PipelineStage::FragmentShader)
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::PipelineStage::FragmentShader)
                                   .enable_depth_test(false)
                                   .vertex_topology(graphics_api::VertexTopology::TriangleStrip)
                                   .push_constant(graphics_api::PipelineStage::FragmentShader, sizeof(PushConstants))
                                   .build())),
    m_descriptorPool(checkResult(m_pipeline.create_descriptor_pool(1, 2, 1))),
    m_sampler(resourceManager.get<ResourceType::Sampler>("linear_repeat_mlod0.sampler"_name)),
    m_descriptors(checkResult(m_descriptorPool.allocate_array(1)))
{
   graphics_api::DescriptorWriter writer(m_device, m_descriptors[0]);
   writer.set_sampled_texture(0, colorTexture, m_sampler);
   writer.set_sampled_texture(1, overlayTexture, m_sampler);
}

void PostProcessingRenderer::update_texture(const graphics_api::Texture &colorTexture,
                                            const graphics_api::Texture &overlayTexture) const
{
   graphics_api::DescriptorWriter writer(m_device, m_descriptors[0]);
   writer.set_sampled_texture(0, colorTexture, m_sampler);
   writer.set_sampled_texture(1, overlayTexture, m_sampler);
}

void PostProcessingRenderer::draw(graphics_api::CommandList &cmdList, const bool enableFXAA) const
{
   cmdList.bind_pipeline(m_pipeline);
   cmdList.bind_descriptor_set(m_descriptors[0]);
   PushConstants constants{
           .enableFXAA = enableFXAA,
   };
   cmdList.push_constant(graphics_api::PipelineStage::FragmentShader, constants);
   cmdList.draw_primitives(4, 0);
}

}// namespace renderer