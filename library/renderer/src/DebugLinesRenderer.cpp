#include "DebugLinesRenderer.hpp"

#include "triglav/graphics_api/CommandList.hpp"
#include "triglav/graphics_api/DescriptorWriter.hpp"
#include "triglav/graphics_api/PipelineBuilder.hpp"
#include "triglav/render_core/RenderCore.hpp"

using namespace triglav::name_literals;
using triglav::ResourceType;
using triglav::render_core::checkResult;
using triglav::resource::ResourceManager;

namespace triglav::renderer {

namespace {

constexpr std::array<std::pair<graphics_api::DescriptorType, u32>, 2> DESCRIPTOR_COUNTS{
   std::pair{graphics_api::DescriptorType::UniformBuffer, 60},
   std::pair{graphics_api::DescriptorType::ImageSampler, 60},
};

}

DebugLinesRenderer::DebugLinesRenderer(graphics_api::Device& device, ResourceManager& resourceManager) :
    m_device(device),
    m_pipeline(checkResult(graphics_api::GraphicsPipelineBuilder(device)
                              .fragment_shader(resourceManager.get("debug_lines.fshader"_rc))
                              .vertex_shader(resourceManager.get("debug_lines.vshader"_rc))
                              .begin_vertex_layout<glm::vec3>()
                              .vertex_attribute(GAPI_FORMAT(RGB, Float32), 0)
                              .end_vertex_layout()
                              .descriptor_binding(graphics_api::DescriptorType::UniformBuffer, graphics_api::PipelineStage::VertexShader)
                              .vertex_topology(graphics_api::VertexTopology::LineList)
                              .rasterization_method(graphics_api::RasterizationMethod::Line)
                              .build())),
    m_descriptorPool(checkResult(m_device.create_descriptor_pool(DESCRIPTOR_COUNTS, 60)))
{
}

DebugLines DebugLinesRenderer::create_line_list(const std::span<glm::vec3> list)
{
   graphics_api::VertexArray<glm::vec3> array(m_device, list.size());
   array.write(list.data(), list.size());

   graphics_api::UniformBuffer<glm::mat4> ubo{m_device};

   graphics_api::DescriptorLayoutArray layouts;
   layouts.add_from_pipeline(m_pipeline);
   auto descriptors = checkResult(m_descriptorPool.allocate_array(layouts));

   graphics_api::DescriptorWriter writer(m_device, descriptors[0]);
   writer.set_uniform_buffer(0, ubo);

   return DebugLines{std::move(array), glm::mat4(1), std::move(ubo), std::move(descriptors)};
}

DebugLines DebugLinesRenderer::create_line_list_from_bounding_box(const geometry::BoundingBox& boundingBox)
{
   std::array<glm::vec3, 24> lines{
      glm::vec3{boundingBox.min.x, boundingBox.min.y, boundingBox.min.z},
      glm::vec3{boundingBox.max.x, boundingBox.min.y, boundingBox.min.z},

      glm::vec3{boundingBox.max.x, boundingBox.min.y, boundingBox.min.z},
      glm::vec3{boundingBox.max.x, boundingBox.max.y, boundingBox.min.z},

      glm::vec3{boundingBox.max.x, boundingBox.max.y, boundingBox.min.z},
      glm::vec3{boundingBox.min.x, boundingBox.max.y, boundingBox.min.z},

      glm::vec3{boundingBox.min.x, boundingBox.max.y, boundingBox.min.z},
      glm::vec3{boundingBox.min.x, boundingBox.min.y, boundingBox.min.z},

      glm::vec3{boundingBox.min.x, boundingBox.min.y, boundingBox.max.z},
      glm::vec3{boundingBox.max.x, boundingBox.min.y, boundingBox.max.z},

      glm::vec3{boundingBox.max.x, boundingBox.min.y, boundingBox.max.z},
      glm::vec3{boundingBox.max.x, boundingBox.max.y, boundingBox.max.z},

      glm::vec3{boundingBox.max.x, boundingBox.max.y, boundingBox.max.z},
      glm::vec3{boundingBox.min.x, boundingBox.max.y, boundingBox.max.z},

      glm::vec3{boundingBox.min.x, boundingBox.max.y, boundingBox.max.z},
      glm::vec3{boundingBox.min.x, boundingBox.min.y, boundingBox.max.z},

      glm::vec3{boundingBox.min.x, boundingBox.min.y, boundingBox.min.z},
      glm::vec3{boundingBox.min.x, boundingBox.min.y, boundingBox.max.z},

      glm::vec3{boundingBox.min.x, boundingBox.max.y, boundingBox.min.z},
      glm::vec3{boundingBox.min.x, boundingBox.max.y, boundingBox.max.z},

      glm::vec3{boundingBox.max.x, boundingBox.min.y, boundingBox.min.z},
      glm::vec3{boundingBox.max.x, boundingBox.min.y, boundingBox.max.z},

      glm::vec3{boundingBox.max.x, boundingBox.max.y, boundingBox.min.z},
      glm::vec3{boundingBox.max.x, boundingBox.max.y, boundingBox.max.z},
   };

   return this->create_line_list(lines);
}

void DebugLinesRenderer::begin_render(graphics_api::CommandList& cmdList) const
{
   cmdList.bind_pipeline(m_pipeline);
}

void DebugLinesRenderer::draw(graphics_api::CommandList& cmdList, const DebugLines& list, const Camera& camera) const
{
   *list.ubo = camera.view_projection_matrix() * list.model;

   cmdList.bind_descriptor_set(graphics_api::PipelineType::Graphics, list.descriptors[0]);
   cmdList.bind_vertex_array(list.array);
   cmdList.draw_primitives(static_cast<int>(list.array.count()), 0);
}

}// namespace triglav::renderer