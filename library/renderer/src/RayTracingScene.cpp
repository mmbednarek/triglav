#include "RayTracingScene.hpp"

#include "triglav/graphics_api/ray_tracing/Geometry.hpp"
#include "triglav/graphics_api/ray_tracing/InstanceBuilder.hpp"
#include "triglav/graphics_api/ray_tracing/RayTracingPipeline.hpp"
#include "triglav/render_core/Model.hpp"
#include "triglav/resource/ResourceManager.hpp"

#include <spdlog/spdlog.h>

namespace triglav::renderer {

namespace gapi = graphics_api;
namespace rt = graphics_api::ray_tracing;
namespace geo = geometry;
using namespace name_literals;

constexpr auto MAX_OBJECT_COUNT = 32;

struct RayTracingConstants
{
   alignas(16) glm::vec3 lightDir;
};

RayTracingScene::RayTracingScene(gapi::Device& device, resource::ResourceManager& resources, Scene& scene) :
    m_device{device},
    m_resourceManager{resources},
    m_teapot(resources.get("teapot.model"_rc)),
    m_scene(scene),
    m_objectBuffer{GAPI_CHECK(
       device.create_buffer(gapi::BufferUsage::StorageBuffer | gapi::BufferUsage::TransferDst, MAX_OBJECT_COUNT * sizeof(ObjectDesc)))},
    m_instanceBuilder(m_device),
    m_scratchHeap(device, gapi::BufferUsage::AccelerationStructure | gapi::BufferUsage::StorageBuffer),
    m_asPool{device},
    m_buildBLContext{device, m_asPool, m_scratchHeap},
    m_buildTLContext{device, m_asPool, m_scratchHeap},
    m_pipeline{GAPI_CHECK(rt::RayTracingPipelineBuilder(device)
                             .ray_generation_shader("rgen"_name, resources.get("rt_general.rgenshader"_rc))
                             .miss_shader("rmiss"_name, resources.get("rt_general.rmissshader"_rc))
                             .miss_shader("shadow_rmiss"_name, resources.get("rt_shadow.rmissshader"_rc))
                             .miss_shader("ao_rmiss"_name, resources.get("rt_ao.rmissshader"_rc))
                             .closest_hit_shader("rchit"_name, resources.get("rt_general.rchitshader"_rc))
                             .closest_hit_shader("shadow_rchit"_name, resources.get("rt_shadow.rchitshader"_rc))
                             .closest_hit_shader("ao_rchit"_name, resources.get("rt_ao.rchitshader"_rc))
                             .general_group("rgen"_name)
                             .general_group("rmiss"_name)
                             .general_group("shadow_rmiss"_name)
                             .general_group("ao_rmiss"_name)
                             .triangle_group("rchit"_name)
                             .triangle_group("shadow_rchit"_name)
                             .triangle_group("ao_rchit"_name)
                             .use_push_descriptors(true)
                             .descriptor_binding(gapi::PipelineStage::RayGenerationShader | gapi::PipelineStage::ClosestHitShader,
                                                 gapi::DescriptorType::AccelerationStructure)
                             .descriptor_binding(gapi::PipelineStage::RayGenerationShader, gapi::DescriptorType::StorageImage)
                             .descriptor_binding(gapi::PipelineStage::RayGenerationShader, gapi::DescriptorType::StorageImage)
                             .descriptor_binding(gapi::PipelineStage::RayGenerationShader, gapi::DescriptorType::UniformBuffer)
                             .descriptor_binding(gapi::PipelineStage::ClosestHitShader, gapi::DescriptorType::StorageBuffer)
                             .push_constant(gapi::PipelineStage::ClosestHitShader, sizeof(RayTracingConstants))
                             .build())},
    m_bindingTable(rt::ShaderBindingTableBuilder(device, m_pipeline)
                      .add_binding("rgen"_name)
                      .add_binding("rmiss"_name)
                      .add_binding("shadow_rmiss"_name)
                      .add_binding("ao_rmiss"_name)
                      .add_binding("rchit"_name)
                      .add_binding("shadow_rchit"_name)
                      .add_binding("ao_rchit"_name)
                      .build()),
    m_ubo(device),
    TG_CONNECT(m_scene, OnObjectAddedToScene, on_object_added_to_scene)
{
}

void RayTracingScene::render(graphics_api::CommandList& cmdList, const graphics_api::Texture& aoTexture,
                             const graphics_api::Texture& shadowsTexture)
{
   if (m_mustUpdateAccelerationStructures) {
      spdlog::info("Updating acceleration structures...");
      m_mustUpdateAccelerationStructures = false;

      auto asUpdateCmdList = GAPI_CHECK(m_device.create_command_list(gapi::WorkType::Compute));

      GAPI_CHECK_STATUS(asUpdateCmdList.begin(gapi::SubmitType::OneTime));
      m_buildBLContext.build_acceleration_structures(asUpdateCmdList);
      GAPI_CHECK_STATUS(asUpdateCmdList.finish());

      GAPI_CHECK_STATUS(m_device.submit_command_list_one_time(asUpdateCmdList));

      m_instanceListBuffer.emplace(m_instanceBuilder.build_buffer());

      m_buildTLContext.add_instance_buffer(*m_instanceListBuffer, m_objects.size());
      m_tlAccelerationStructure = m_buildTLContext.commit_instances();

      GAPI_CHECK_STATUS(asUpdateCmdList.begin(gapi::SubmitType::OneTime));
      m_buildTLContext.build_acceleration_structures(asUpdateCmdList);
      GAPI_CHECK_STATUS(asUpdateCmdList.finish());

      GAPI_CHECK_STATUS(m_device.submit_command_list_one_time(asUpdateCmdList));

      GAPI_CHECK_STATUS(m_objectBuffer.write_indirect(m_objects.data(), m_objects.size() * sizeof(ObjectDesc)));
   }

   {
      const auto lk{m_ubo.lock()};
      lk->viewInverse = glm::inverse(m_scene.camera().view_matrix());
      lk->projInverse = glm::inverse(m_scene.camera().projection_matrix());
   }

   m_ubo.sync(cmdList);
   cmdList.execution_barrier(graphics_api::PipelineStage::Transfer, graphics_api::PipelineStage::RayGenerationShader);

   cmdList.bind_pipeline(m_pipeline);
   cmdList.bind_acceleration_structure(0, *m_tlAccelerationStructure);
   cmdList.bind_storage_image(1, aoTexture);
   cmdList.bind_storage_image(2, shadowsTexture);
   cmdList.bind_uniform_buffer(3, m_ubo);
   cmdList.bind_storage_buffer(4, m_objectBuffer);
   RayTracingConstants constants{.lightDir{m_scene.shadow_map_camera(0).orientation() * glm::vec3(0.0f, 1.0f, 0.0f)}};
   cmdList.push_constant(graphics_api::PipelineStage::ClosestHitShader, constants);
   cmdList.trace_rays(m_bindingTable, {aoTexture.width(), aoTexture.height(), 1});
}

void RayTracingScene::on_object_added_to_scene(const SceneObject& object)
{
   m_mustUpdateAccelerationStructures = true;

   const auto& model = m_resourceManager.get(object.model);
   const auto vertexCount = model.mesh.vertices.count();
   const auto triangleCount = model.mesh.indices.count() / 3;
   m_buildBLContext.add_triangle_buffer(model.mesh.vertices.buffer(), model.mesh.indices.buffer(), GAPI_FORMAT(RGB, Float32),
                                        sizeof(geo::Vertex), vertexCount, triangleCount);

   auto* accStruct = m_buildBLContext.commit_triangles();

   m_instanceBuilder.add_instance(*accStruct, object.model_matrix(), m_objects.size());

   m_objects.emplace_back(ObjectDesc{.indexBuffer = model.mesh.indices.buffer().buffer_address(),
                                     .vertexBuffer = model.mesh.vertices.buffer().buffer_address()});
}


}// namespace triglav::renderer
