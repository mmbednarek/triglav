#include "RayTracingScene.hpp"

#include "triglav/graphics_api/ray_tracing/Geometry.hpp"
#include "triglav/graphics_api/ray_tracing/InstanceBuilder.hpp"
#include "triglav/graphics_api/ray_tracing/RayTracingPipeline.hpp"
#include "triglav/render_objects/Mesh.hpp"
#include "triglav/resource/ResourceManager.hpp"

namespace triglav::renderer {

namespace gapi = graphics_api;
namespace rt = graphics_api::ray_tracing;
namespace geo = geometry;
using namespace name_literals;

constexpr auto MAX_OBJECT_COUNT = 32;

struct RayTracingConstants
{
   alignas(16) glm::vec3 light_dir;
};

RayTracingScene::RayTracingScene(gapi::Device& device, resource::ResourceManager& resources, Scene& scene) :
    m_device{device},
    m_resource_manager{resources},
    m_teapot(resources.get("mesh/teapot.mesh"_rc)),
    m_scene(scene),
    m_object_buffer{GAPI_CHECK(
       device.create_buffer(gapi::BufferUsage::StorageBuffer | gapi::BufferUsage::TransferDst, MAX_OBJECT_COUNT * sizeof(ObjectDesc)))},
    m_instance_builder(m_device),
    m_scratch_heap(device, gapi::BufferUsage::AccelerationStructure | gapi::BufferUsage::StorageBuffer),
    m_as_pool{device},
    m_build_blcontext{device, m_as_pool, m_scratch_heap},
    m_build_tlcontext{device, m_as_pool, m_scratch_heap},
    TG_CONNECT(m_scene, OnObjectAddedToScene, on_object_added_to_scene)
{
}

void RayTracingScene::build_acceleration_structures()
{
   if (!m_must_update_acceleration_structures) {
      return;
   }

   log_info("Updating acceleration structures...");
   m_must_update_acceleration_structures = false;

   auto as_update_cmd_list = GAPI_CHECK(m_device.create_command_list(gapi::WorkType::Compute));

   GAPI_CHECK_STATUS(as_update_cmd_list.begin(gapi::SubmitType::OneTime));
   m_build_blcontext.build_acceleration_structures(as_update_cmd_list);
   GAPI_CHECK_STATUS(as_update_cmd_list.finish());

   GAPI_CHECK_STATUS(m_device.submit_command_list_one_time(as_update_cmd_list));

   m_instance_list_buffer.emplace(m_instance_builder.build_buffer());

   m_build_tlcontext.add_instance_buffer(*m_instance_list_buffer, static_cast<u32>(m_objects.size()));
   m_tl_acceleration_structure = m_build_tlcontext.commit_instances();

   GAPI_CHECK_STATUS(as_update_cmd_list.begin(gapi::SubmitType::OneTime));
   m_build_tlcontext.build_acceleration_structures(as_update_cmd_list);
   GAPI_CHECK_STATUS(as_update_cmd_list.finish());

   GAPI_CHECK_STATUS(m_device.submit_command_list_one_time(as_update_cmd_list));

   GAPI_CHECK_STATUS(m_object_buffer.write_indirect(m_objects.data(), m_objects.size() * sizeof(ObjectDesc)));
}

void RayTracingScene::on_object_added_to_scene(const SceneObject& object)
{
   m_must_update_acceleration_structures = true;

   const auto& model = m_resource_manager.get(object.model);
   const auto vertex_count = static_cast<u32>(model.mesh.vertices.count());
   const auto triangle_count = static_cast<u32>(model.mesh.indices.count() / 3);
   m_build_blcontext.add_triangle_buffer(model.mesh.vertices.buffer(), model.mesh.indices.buffer(), GAPI_FORMAT(RGB, Float32),
                                         sizeof(geo::Vertex), vertex_count, triangle_count);

   auto* acc_struct = m_build_blcontext.commit_triangles();

   m_instance_builder.add_instance(*acc_struct, object.model_matrix(), m_objects.size());

   m_objects.emplace_back(ObjectDesc{.index_buffer = model.mesh.indices.buffer().buffer_address(),
                                     .vertex_buffer = model.mesh.vertices.buffer().buffer_address()});
}


}// namespace triglav::renderer
