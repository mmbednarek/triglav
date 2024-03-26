#include "GroundRenderer.h"

#include "triglav/graphics_api/CommandList.h"
#include "triglav/graphics_api/DescriptorWriter.h"
#include "triglav/graphics_api/PipelineBuilder.h"
#include "triglav/render_core/RenderCore.hpp"

using namespace triglav::name_literals;
using triglav::ResourceType;
using triglav::render_core::checkResult;
using triglav::resource::ResourceManager;

namespace triglav::renderer {

GroundRenderer::GroundRenderer(graphics_api::Device &device, graphics_api::RenderTarget &renderTarget,
                               ResourceManager &resourceManager) :
    m_device(device),
    m_pipeline(checkResult(graphics_api::PipelineBuilder(m_device, renderTarget)
                                   .fragment_shader(resourceManager.get<ResourceType::FragmentShader>("ground.fshader"_name))
                                   .vertex_shader(resourceManager.get<ResourceType::VertexShader>("ground.vshader"_name))
                                   // Descriptor layout
                                   .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                                       graphics_api::PipelineStage::VertexShader)
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::PipelineStage::FragmentShader)
                                   .enable_depth_test(true)
                                   .enable_blending(false)
                                   .vertex_topology(graphics_api::VertexTopology::TriangleStrip)
                                   .build())),
    m_descriptorPool(checkResult(m_pipeline.create_descriptor_pool(1, 1, 1))),
    m_descriptors(checkResult(m_descriptorPool.allocate_array(1))),
    m_uniformBuffer(m_device),
    m_sampler(resourceManager.get<ResourceType::Sampler>("ground_sampler.sampler"_name))
{
   m_uniformBuffer->model = glm::scale(glm::mat4(1), glm::vec3{200, 200, 200});

   graphics_api::DescriptorWriter writer(m_device, m_descriptors[0]);
   writer.set_uniform_buffer(0, m_uniformBuffer);
   writer.set_sampled_texture(1, resourceManager.get<ResourceType::Texture>("board.tex"_name), m_sampler);
}

void GroundRenderer::draw(graphics_api::CommandList &cmdList, const Camera &camera) const
{
   m_uniformBuffer->view = camera.view_matrix();
   m_uniformBuffer->proj = camera.projection_matrix();

   cmdList.bind_pipeline(m_pipeline);
   cmdList.bind_descriptor_set(m_descriptors[0]);
   cmdList.draw_primitives(4, 0);
}

}// namespace renderer