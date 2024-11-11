#include "GroundRenderer.hpp"

#include "triglav/graphics_api/CommandList.hpp"
#include "triglav/graphics_api/DescriptorWriter.hpp"
#include "triglav/graphics_api/PipelineBuilder.hpp"
#include "triglav/render_core/RenderCore.hpp"

using namespace triglav::name_literals;
using triglav::ResourceType;
using triglav::render_core::checkResult;
using triglav::resource::ResourceManager;

namespace triglav::renderer {

GroundRenderer::GroundRenderer(graphics_api::Device& device, graphics_api::RenderTarget& renderTarget, ResourceManager& resourceManager) :
    m_device(device),
    m_pipeline(checkResult(graphics_api::GraphicsPipelineBuilder(m_device, renderTarget)
                              .fragment_shader(resourceManager.get("ground.fshader"_rc))
                              .vertex_shader(resourceManager.get("ground.vshader"_rc))
                              // Descriptor layout
                              .descriptor_binding(graphics_api::DescriptorType::UniformBuffer, graphics_api::PipelineStage::VertexShader)
                              .descriptor_binding(graphics_api::DescriptorType::ImageSampler, graphics_api::PipelineStage::FragmentShader)
                              .enable_depth_test(true)
                              .enable_blending(false)
                              .use_push_descriptors(true)
                              .vertex_topology(graphics_api::VertexTopology::TriangleStrip)
                              .build())),
    m_texture(resourceManager.get("board.tex"_rc))
{
}

void GroundRenderer::prepare_resources(graphics_api::CommandList& cmdList, UniformBuffer& ubo, const Camera& camera)
{
   {
      auto lock = ubo.lock();
      lock->view = camera.view_matrix();
      lock->proj = camera.projection_matrix();
   }

   ubo.sync(cmdList);
}

void GroundRenderer::draw(graphics_api::CommandList& cmdList, UniformBuffer& ubo) const
{
   cmdList.bind_pipeline(m_pipeline);

   cmdList.bind_uniform_buffer(0, ubo);
   cmdList.bind_texture(1, m_texture);
   cmdList.draw_primitives(4, 0);
}

}// namespace triglav::renderer