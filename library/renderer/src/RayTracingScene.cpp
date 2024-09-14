#include "RayTracingScene.hpp"

#include "triglav/geometry/DebugMesh.h"
#include "triglav/resource/ResourceManager.h"
#include "triglav/graphics_api/ray_tracing/Geometry.hpp"
#include "triglav/graphics_api/ray_tracing/InstanceBuilder.hpp"
#include "triglav/graphics_api/ray_tracing/RayTracingPipeline.hpp"

namespace triglav::renderer {

namespace gapi = graphics_api;
namespace rt = graphics_api::ray_tracing;
namespace geo = geometry;
using namespace name_literals;

RayTracingScene::RayTracingScene(gapi::Device& device, resource::ResourceManager& resources) :
    m_device{device},
    m_exampleMesh{geo::create_box(geo::Extent3D{2.0f, 2.0f, 2.0f})},
    m_boundingBoxBuffer{GAPI_CHECK(device.create_buffer(gapi::BufferUsage::AccelerationStructureRead | gapi::BufferUsage::TransferDst, sizeof(VkAabbPositionsKHR)))},
    m_asPool{device},
    m_buildBLContext{device, m_asPool},
    m_buildTLContext{device, m_asPool},
    m_pipeline{GAPI_CHECK(rt::RayTracingPipelineBuilder(device)
                             .ray_generation_shader("rgen"_name, resources.get("rt_general.rgenshader"_rc))
                             .closest_hit_shader("rchit"_name, resources.get("rt_general.rchitshader"_rc))
                             .miss_shader("rmiss"_name, resources.get("rt_general.rmissshader"_rc))
                             .general_group("rmiss"_name)
                             .triangle_group("rmiss"_name, "rchit"_name)
                             .use_push_descriptors(true)
                             .descriptor_binding(gapi::PipelineStage::RayGenerationShader, gapi::DescriptorType::AccelerationStructure)
                             .descriptor_binding(gapi::PipelineStage::RayGenerationShader, gapi::DescriptorType::StorageImage)
                             .descriptor_binding(gapi::PipelineStage::RayGenerationShader, gapi::DescriptorType::UniformBuffer)
                             .build()
   )}
{
   VkAabbPositionsKHR positions{
      .minX=-1.0f,
      .minY=-1.0f,
      .minZ=-1.0f,
      .maxX=1.0f,
      .maxY=1.0f,
      .maxZ=1.0f,
   };
   GAPI_CHECK_STATUS(m_boundingBoxBuffer.write_indirect(&positions, sizeof(VkAabbPositionsKHR)));

   m_exampleMesh.triangulate();
   m_examplesMeshVertexData.emplace(m_exampleMesh.upload_to_device(device, gapi::BufferUsage::AccelerationStructureRead));

   m_buildBLContext.add_bounding_box_buffer(m_boundingBoxBuffer, sizeof(VkAabbPositionsKHR), 1);
   m_buildBLContext.commit_bounding_boxes();

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
   m_buildTLContext.commit_instances();

   GAPI_CHECK_STATUS(cmdList.begin(gapi::SubmitType::OneTime));
   m_buildTLContext.build_acceleration_structures(cmdList);
   GAPI_CHECK_STATUS(cmdList.finish());

   GAPI_CHECK_STATUS(m_device.submit_command_list_one_time(cmdList));
}

}// namespace triglav::renderer
