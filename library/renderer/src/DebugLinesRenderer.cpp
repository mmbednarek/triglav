#include "DebugLinesRenderer.h"

#include "graphics_api/CommandList.h"
#include "graphics_api/DescriptorWriter.h"
#include "graphics_api/PipelineBuilder.h"

using namespace triglav::name_literals;
using triglav::ResourceType;
using triglav::resource::ResourceManager;
using triglav::render_core::checkResult;

namespace renderer {

DebugLinesRenderer::DebugLinesRenderer(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
                                       ResourceManager &resourceManager) :
    m_device(device),
    m_pipeline(checkResult(graphics_api::PipelineBuilder(device, renderPass)
                                   .fragment_shader(resourceManager.get<ResourceType::FragmentShader>("debug_lines.fshader"_name))
                                   .vertex_shader(resourceManager.get<ResourceType::VertexShader>("debug_lines.vshader"_name))
                                   .begin_vertex_layout<glm::vec3>()
                                   .vertex_attribute(GAPI_FORMAT(RGB, Float32), 0)
                                   .end_vertex_layout()
                                   .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                                       graphics_api::ShaderStage::Vertex)
                                   .vertex_topology(graphics_api::VertexTopology::LineList)
                                   .razterization_method(graphics_api::RasterizationMethod::Line)
                                   .build())),
    m_descriptorPool(checkResult(m_pipeline.create_descriptor_pool(20, 1, 20)))
{
}

DebugLines DebugLinesRenderer::create_line_list(const std::span<glm::vec3> list)
{
   graphics_api::VertexArray<glm::vec3> array(m_device, list.size());
   array.write(list.data(), list.size());

   graphics_api::UniformBuffer<glm::mat4> ubo{m_device};

   auto descriptors = checkResult(m_descriptorPool.allocate_array(1));

   graphics_api::DescriptorWriter writer(m_device, descriptors[0]);
   writer.set_uniform_buffer(0, ubo);

   return DebugLines{std::move(array), glm::mat4(1), std::move(ubo), std::move(descriptors)};
}

DebugLines DebugLinesRenderer::create_line_list_from_bouding_box(const geometry::BoundingBox &boudingBox)
{
   std::array<glm::vec3, 24> lines{
           glm::vec3{boudingBox.min.x, boudingBox.min.y, boudingBox.min.z},
           glm::vec3{boudingBox.max.x, boudingBox.min.y, boudingBox.min.z},

           glm::vec3{boudingBox.max.x, boudingBox.min.y, boudingBox.min.z},
           glm::vec3{boudingBox.max.x, boudingBox.max.y, boudingBox.min.z},

           glm::vec3{boudingBox.max.x, boudingBox.max.y, boudingBox.min.z},
           glm::vec3{boudingBox.min.x, boudingBox.max.y, boudingBox.min.z},

           glm::vec3{boudingBox.min.x, boudingBox.max.y, boudingBox.min.z},
           glm::vec3{boudingBox.min.x, boudingBox.min.y, boudingBox.min.z},

           glm::vec3{boudingBox.min.x, boudingBox.min.y, boudingBox.max.z},
           glm::vec3{boudingBox.max.x, boudingBox.min.y, boudingBox.max.z},

           glm::vec3{boudingBox.max.x, boudingBox.min.y, boudingBox.max.z},
           glm::vec3{boudingBox.max.x, boudingBox.max.y, boudingBox.max.z},

           glm::vec3{boudingBox.max.x, boudingBox.max.y, boudingBox.max.z},
           glm::vec3{boudingBox.min.x, boudingBox.max.y, boudingBox.max.z},

           glm::vec3{boudingBox.min.x, boudingBox.max.y, boudingBox.max.z},
           glm::vec3{boudingBox.min.x, boudingBox.min.y, boudingBox.max.z},

           glm::vec3{boudingBox.min.x, boudingBox.min.y, boudingBox.min.z},
           glm::vec3{boudingBox.min.x, boudingBox.min.y, boudingBox.max.z},

           glm::vec3{boudingBox.min.x, boudingBox.max.y, boudingBox.min.z},
           glm::vec3{boudingBox.min.x, boudingBox.max.y, boudingBox.max.z},

           glm::vec3{boudingBox.max.x, boudingBox.min.y, boudingBox.min.z},
           glm::vec3{boudingBox.max.x, boudingBox.min.y, boudingBox.max.z},

           glm::vec3{boudingBox.max.x, boudingBox.max.y, boudingBox.min.z},
           glm::vec3{boudingBox.max.x, boudingBox.max.y, boudingBox.max.z},
   };

   return this->create_line_list(lines);
}

void DebugLinesRenderer::begin_render(graphics_api::CommandList &cmdList) const
{
   cmdList.bind_pipeline(m_pipeline);
}

void DebugLinesRenderer::draw(const graphics_api::CommandList &cmdList, const DebugLines &list, const Camera& camera) const
{
   *list.ubo = camera.view_projection_matrix() * list.model;

   cmdList.bind_descriptor_set(list.descriptors[0]);
   cmdList.bind_vertex_array(list.array);
   cmdList.draw_primitives(static_cast<int>(list.array.count()), 0);
}

}// namespace renderer