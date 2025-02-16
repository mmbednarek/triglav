#include "RayTracingScene.hpp"

#include "triglav/graphics_api/ray_tracing/Geometry.hpp"
#include "triglav/graphics_api/ray_tracing/InstanceBuilder.hpp"
#include "triglav/graphics_api/ray_tracing/RayTracingPipeline.hpp"
#include "triglav/render_objects/Mesh.hpp"
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
    m_teapot(resources.get("teapot.mesh"_rc)),
    m_scene(scene),
    m_objectBuffer{GAPI_CHECK(
       device.create_buffer(gapi::BufferUsage::StorageBuffer | gapi::BufferUsage::TransferDst, MAX_OBJECT_COUNT * sizeof(ObjectDesc)))},
    m_instanceBuilder(m_device),
    m_scratchHeap(device, gapi::BufferUsage::AccelerationStructure | gapi::BufferUsage::StorageBuffer),
    m_asPool{device},
    m_buildBLContext{device, m_asPool, m_scratchHeap},
    m_buildTLContext{device, m_asPool, m_scratchHeap},
    TG_CONNECT(m_scene, OnObjectAddedToScene, on_object_added_to_scene)
{
}

void RayTracingScene::build_acceleration_structures()
{
   if (!m_mustUpdateAccelerationStructures) {
      return;
   }

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
