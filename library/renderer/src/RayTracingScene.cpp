#include "RayTracingScene.hpp"

#include "triglav/geometry/DebugMesh.h"
#include "triglav/graphics_api/ray_tracing/Geometry.hpp"
#include "triglav/graphics_api/ray_tracing/InstanceBuilder.hpp"
#include "triglav/graphics_api/ray_tracing/RayTracingPipeline.hpp"
#include "triglav/resource/ResourceManager.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace triglav::renderer {

namespace gapi = graphics_api;
namespace rt = graphics_api::ray_tracing;
namespace geo = geometry;
using namespace name_literals;

RayTracingScene::RayTracingScene(gapi::Device& device, resource::ResourceManager& resources) :
    m_device{device},
    m_exampleMesh{geo::create_sphere(16, 16, 3.0f)},
    m_boundingBoxBuffer{GAPI_CHECK(
       device.create_buffer(gapi::BufferUsage::AccelerationStructureRead | gapi::BufferUsage::TransferDst, sizeof(VkAabbPositionsKHR)))},
    m_asPool{device},
    m_buildBLContext{device, m_asPool},
    m_buildTLContext{device, m_asPool},
    m_pipeline{GAPI_CHECK(rt::RayTracingPipelineBuilder(device)
                             .ray_generation_shader("rgen"_name, resources.get("rt_general.rgenshader"_rc))
                             .miss_shader("rmiss"_name, resources.get("rt_general.rmissshader"_rc))
                             .closest_hit_shader("rchit"_name, resources.get("rt_general.rchitshader"_rc))
                             .general_group("rgen"_name)
                             .general_group("rmiss"_name)
                             .triangle_group("rchit"_name)
                             .use_push_descriptors(true)
                             .descriptor_binding(gapi::PipelineStage::RayGenerationShader, gapi::DescriptorType::AccelerationStructure)
                             .descriptor_binding(gapi::PipelineStage::RayGenerationShader, gapi::DescriptorType::StorageImage)
                             .descriptor_binding(gapi::PipelineStage::RayGenerationShader, gapi::DescriptorType::UniformBuffer)
                             .build())},
    m_bindingTable(rt::ShaderBindingTableBuilder(device, m_pipeline)
                      .add_binding("rgen"_name)
                      .add_binding("rmiss"_name)
                      .add_binding("rchit"_name)
                      .build()),
    m_ubo(device)
{
//   VkAabbPositionsKHR positions{
//      .minX = -1.0f,
//      .minY = -1.0f,
//      .minZ = -1.0f,
//      .maxX = 1.0f,
//      .maxY = 1.0f,
//      .maxZ = 1.0f,
//   };
//   GAPI_CHECK_STATUS(m_boundingBoxBuffer.write_indirect(&positions, sizeof(VkAabbPositionsKHR)));

   m_exampleMesh.triangulate();
   m_examplesMeshVertexData.emplace(m_exampleMesh.upload_to_device(device, gapi::BufferUsage::AccelerationStructureRead));

//   m_buildBLContext.add_bounding_box_buffer(m_boundingBoxBuffer, sizeof(VkAabbPositionsKHR), 1);
//   m_buildBLContext.commit_bounding_boxes();

   auto vertexCount = m_examplesMeshVertexData->mesh.vertices.count();
   auto triangleCount = m_examplesMeshVertexData->mesh.indices.count() / 3;
   m_buildBLContext.add_triangle_buffer(m_examplesMeshVertexData->mesh.vertices.buffer(), m_examplesMeshVertexData->mesh.indices.buffer(),
                                        GAPI_FORMAT(RGB, Float32), sizeof(geo::Vertex), vertexCount, triangleCount);
   auto* trianglesStruct = m_buildBLContext.commit_triangles();

   auto cmdList = GAPI_CHECK(m_device.create_command_list(gapi::WorkType::Compute));

   GAPI_CHECK_STATUS(cmdList.begin(gapi::SubmitType::OneTime));
   m_buildBLContext.build_acceleration_structures(cmdList);
   GAPI_CHECK_STATUS(cmdList.finish());

   GAPI_CHECK_STATUS(m_device.submit_command_list_one_time(cmdList));

   rt::InstanceBuilder instanceBuilder(m_device);
   instanceBuilder.add_instance(*trianglesStruct, glm::mat4(1));

   m_instanceListBuffer.emplace(instanceBuilder.build_buffer());

   m_buildTLContext.add_instance_buffer(*m_instanceListBuffer, 1);
   m_tlAccelerationStructure = m_buildTLContext.commit_instances();

   GAPI_CHECK_STATUS(cmdList.begin(gapi::SubmitType::OneTime));
   m_buildTLContext.build_acceleration_structures(cmdList);
   GAPI_CHECK_STATUS(cmdList.finish());

   GAPI_CHECK_STATUS(m_device.submit_command_list_one_time(cmdList));
}

void RayTracingScene::render(graphics_api::CommandList& cmdList, const graphics_api::Texture& texture, Camera& camera)
{
   {
      auto lk{m_ubo.lock()};
      lk->viewInverse = glm::inverse(camera.view_matrix());
      lk->projInverse = glm::inverse(camera.projection_matrix());
   }

   m_ubo.sync(cmdList);
   cmdList.execution_barrier(graphics_api::PipelineStage::Transfer, graphics_api::PipelineStage::RayGenerationShader);

   cmdList.bind_pipeline(m_pipeline);
   cmdList.bind_acceleration_structure(0, *m_tlAccelerationStructure);
   cmdList.bind_storage_image(1, texture);
   cmdList.bind_uniform_buffer(2, m_ubo);
   cmdList.trace_rays(m_bindingTable, {texture.width(), texture.height(), 1});
}

}// namespace triglav::renderer
