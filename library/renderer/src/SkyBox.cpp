#include "SkyBox.h"

#include "geometry/DebugMesh.h"
#include "Renderer.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace {

graphics_api::Mesh<geometry::Vertex> create_skybox_mesh(graphics_api::Device& device)
{
   auto mesh = geometry::create_box({50, 50, 50});
   mesh.reverse_orientation();
   mesh.triangulate();
   return mesh.upload_to_device(device);
}

}

namespace renderer {

SkyBox::SkyBox(Renderer &renderer) :
    m_fragmentShader(
            renderer.load_shader(graphics_api::ShaderStage::Fragment, "shader/skybox/fragment.spv")),
    m_vertexShader(renderer.load_shader(graphics_api::ShaderStage::Vertex, "shader/skybox/vertex.spv")),
    m_texture(renderer.load_texture("texture/skybox.png")),
    m_mesh(create_skybox_mesh(renderer.device())),
    m_pipeline(
            checkResult(renderer.create_pipeline()
                                .fragment_shader(m_fragmentShader)
                                .vertex_shader(m_vertexShader)
                                // Vertex description
                                .begin_vertex_layout<geometry::Vertex>()
                                .vertex_attribute(GAPI_COLOR_FORMAT(RGB, Float32), offsetof(geometry::Vertex, location))
                                .vertex_attribute(GAPI_COLOR_FORMAT(RG, Float32), offsetof(geometry::Vertex, uv))
                                .vertex_attribute(GAPI_COLOR_FORMAT(RGB, Float32), offsetof(geometry::Vertex, normal))
                                .end_vertex_layout()
                                // Descriptor layout
                                .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                                    graphics_api::ShaderStage::Vertex)
                                .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                    graphics_api::ShaderStage::Fragment)
                                .descriptor_budget(3)
                                .enable_depth_test(false)
                                .build()

                                )),
    m_descGroup(checkResult(m_pipeline.allocate_descriptors(1))),
    m_uniformBuffer(renderer.create_ubo_buffer<SkyBoxUBO>()),
    m_uniformBufferMapping(checkResult(m_uniformBuffer.map_memory()))
{
   std::array<graphics_api::DescriptorWrite, 2> writes{
           graphics_api::DescriptorWrite{
                                         .type    = graphics_api::DescriptorType::UniformBuffer,
                                         .binding = 0,
                                         .data    = &m_uniformBuffer,
                                         },
           {
                                         .type    = graphics_api::DescriptorType::ImageSampler,
                                         .binding = 1,
                                         .data    = &m_texture,
                                         }
   };

   m_descGroup.update(0, writes);
}

void SkyBox::on_render(const graphics_api::CommandList& commandList, float yaw, float pitch, float width, float height) const
{
   static constexpr auto yVector = glm::vec4{0.0f, 1.0f, 0, 1.0f};
   const auto yawMatrix   = glm::rotate(glm::mat4(1), yaw, glm::vec3{0.0f, 0.0f, 1.0f});
   const auto pitchMatrix = glm::rotate(glm::mat4(1), pitch, glm::vec3{1.0f, 0.0f, 0.0f});
   const auto forwardVector = glm::vec3(yawMatrix * pitchMatrix * yVector);

   const auto view = glm::lookAt(glm::vec3{0, 0, 0}, forwardVector, glm::vec3{0.0f, 0.0f, 1.0f});
   const auto projection = glm::perspective(glm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);

   SkyBoxUBO object1{};
   object1.view  = view;
   object1.proj  = projection;
   m_uniformBufferMapping.write(&object1, sizeof(UniformBufferObject));

   commandList.bind_pipeline(m_pipeline);
   commandList.bind_descriptor_group(m_descGroup, 0);
   commandList.draw_mesh(m_mesh);
}

}// namespace renderer