#include "ShadingRenderer.h"

#include <cstring>
#include <random>

#include "triglav/graphics_api/CommandList.h"
#include "triglav/graphics_api/DescriptorWriter.h"
#include "triglav/graphics_api/Framebuffer.h"
#include "triglav/graphics_api/PipelineBuilder.h"
#include "triglav/render_core/RenderCore.hpp"

using namespace triglav::name_literals;
using triglav::ResourceType;
using triglav::render_core::checkResult;
using triglav::resource::ResourceManager;

namespace triglav::renderer {

ShadingRenderer::ShadingRenderer(graphics_api::Device &device, graphics_api::RenderTarget &renderTarget,
                                 ResourceManager &resourceManager, graphics_api::Framebuffer &geometryBuffer,
                                 const graphics_api::Texture &shadowMapTexture) :
    m_device(device),
    m_pipeline(checkResult(
            graphics_api::PipelineBuilder(m_device, renderTarget)
                    .fragment_shader(
                            resourceManager.get<ResourceType::FragmentShader>("shading.fshader"_name))
                    .vertex_shader(resourceManager.get<ResourceType::VertexShader>("shading.vshader"_name))
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
                    .use_push_descriptors(true)
                    .enable_depth_test(false)
                    .vertex_topology(graphics_api::VertexTopology::TriangleStrip)
                    .build())),
    // m_descriptorPool(checkResult(m_pipeline.create_descriptor_pool(1, 5, 1))),
    m_sampler(resourceManager.get<ResourceType::Sampler>("linear_repeat_mlod0.sampler"_name)),
    // m_descriptors(checkResult(m_descriptorPool.allocate_array(1))),
    m_uniformBuffer(m_device),
    m_geometryBuffer(geometryBuffer),
    m_shadowMapTexture(shadowMapTexture)
{
}

void ShadingRenderer::draw(render_core::FrameResources &resources, graphics_api::CommandList &cmdList,
                           const glm::vec3 &lightPosition, const glm::mat4 &shadowMapMat,
                           const bool ssaoEnabled) const
{
   m_uniformBuffer->shadowMapMat = shadowMapMat;

   cmdList.bind_pipeline(m_pipeline);

   auto &aoNode = resources.node<render_core::NodeFrameResources>("ambient_occlusion"_name_id);

   graphics_api::DescriptorWriter writer(m_device);
   writer.set_sampled_texture(0, m_geometryBuffer.texture(0), m_sampler);
   writer.set_sampled_texture(1, m_geometryBuffer.texture(1), m_sampler);
   writer.set_sampled_texture(2, m_geometryBuffer.texture(2), m_sampler);
   writer.set_sampled_texture(3, aoNode.framebuffer("ao"_name_id).texture(0), m_sampler);
   writer.set_sampled_texture(4, m_shadowMapTexture, m_sampler);
   writer.set_uniform_buffer(5, m_uniformBuffer);
   cmdList.push_descriptors(0, writer);

   PushConstant pushConstant{
           .lightPosition = lightPosition,
           .enableSSAO    = ssaoEnabled,
   };
   cmdList.push_constant(graphics_api::PipelineStage::FragmentShader, pushConstant);

   cmdList.draw_primitives(4, 0);
}

}// namespace triglav::renderer