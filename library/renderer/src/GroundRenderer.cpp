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
    m_pipeline(checkResult(graphics_api::GraphicsPipelineBuilder(m_device, renderTarget)
                                   .fragment_shader(resourceManager.get("ground.fshader"_rc))
                                   .vertex_shader(resourceManager.get("ground.vshader"_rc))
                                   // Descriptor layout
                                   .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                                       graphics_api::PipelineStage::VertexShader)
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::PipelineStage::FragmentShader)
                                   .enable_depth_test(true)
                                   .enable_blending(false)
                                   .use_push_descriptors(true)
                                   .vertex_topology(graphics_api::VertexTopology::TriangleStrip)
                                   .build())),
    m_sampler(resourceManager.get("ground_sampler.sampler"_rc)),
    m_texture(resourceManager.get("board.tex"_rc))
{
}

void GroundRenderer::draw(graphics_api::CommandList &cmdList, UniformBuffer& ubo, const Camera &camera) const
{
   ubo->view = camera.view_matrix();
   ubo->proj = camera.projection_matrix();

   cmdList.bind_pipeline(m_pipeline);

   graphics_api::DescriptorWriter writer(m_device);
   writer.set_uniform_buffer(0, ubo);
   writer.set_sampled_texture(1, m_texture, m_sampler);
   cmdList.push_descriptors(0, writer, graphics_api::PipelineType::Graphics);

   cmdList.draw_primitives(4, 0);
}

}// namespace renderer