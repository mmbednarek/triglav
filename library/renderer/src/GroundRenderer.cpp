#include "GroundRenderer.h"

#include "graphics_api/CommandList.h"
#include "graphics_api/DescriptorWriter.h"
#include "graphics_api/PipelineBuilder.h"

namespace renderer {

GroundRenderer::GroundRenderer(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
                               const ResourceManager &resourceManager) :
    m_device(device),
    m_pipeline(checkResult(graphics_api::PipelineBuilder(m_device, renderPass)
                                   .fragment_shader(resourceManager.shader("fsh:ground"_name))
                                   .vertex_shader(resourceManager.shader("vsh:ground"_name))
                                   // Vertex description
                                   .begin_vertex_layout<geometry::Vertex>()
                                   .end_vertex_layout()
                                   // Descriptor layout
                                   .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                                       graphics_api::ShaderStage::Vertex)
                                   .enable_depth_test(true)
                                   .enable_blending(false)
                                   .vertex_topology(graphics_api::VertexTopology::TriangleStrip)
                                   .build())),
    m_descriptorPool(checkResult(m_pipeline.create_descriptor_pool(1, 1, 1))),
    m_descriptors(checkResult(m_descriptorPool.allocate_array(1))),
    m_uniformBuffer(m_device)
{
   m_uniformBuffer->model = glm::scale(glm::mat4(1), glm::vec3{100, 100, 100});

   graphics_api::DescriptorWriter writer(m_device, m_descriptors[0]);
   writer.set_uniform_buffer(0, m_uniformBuffer);
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