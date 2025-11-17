#pragma once

#include "Camera.hpp"
#include "Scene.hpp"

#include "triglav/Logging.hpp"
#include "triglav/geometry/Mesh.hpp"
#include "triglav/graphics_api/ReplicatedBuffer.hpp"
#include "triglav/graphics_api/ray_tracing/AccelerationStructurePool.hpp"
#include "triglav/graphics_api/ray_tracing/Geometry.hpp"
#include "triglav/graphics_api/ray_tracing/InstanceBuilder.hpp"
#include "triglav/graphics_api/ray_tracing/RayTracingPipeline.hpp"
#include "triglav/graphics_api/ray_tracing/ShaderBindingTable.hpp"

#include <glm/mat4x4.hpp>
#include <utility>

namespace triglav::resource {
class ResourceManager;
}

namespace triglav::renderer {

constexpr auto AO_POINT_COUNT = 64;

namespace stage {
class RayTracingStage;
}

struct RayGenerationUboData
{
   glm::mat4 view_inverse;
   glm::mat4 proj_inverse;
};
using RayGenerationUbo = graphics_api::UniformReplicatedBuffer<RayGenerationUboData>;

struct ObjectDesc
{
   graphics_api::BufferAddress index_buffer;
   graphics_api::BufferAddress vertex_buffer;
};

class RayTracingScene
{
   TG_DEFINE_LOG_CATEGORY(RayTracingScene)
   friend stage::RayTracingStage;

 public:
   using Self = RayTracingScene;

   struct AmbientOcclusionPoints
   {
      glm::vec4 points[AO_POINT_COUNT];
   };

   explicit RayTracingScene(graphics_api::Device& device, resource::ResourceManager& resources, Scene& scene);

   void render(graphics_api::CommandList& cmd_list, const graphics_api::Texture& texture, const graphics_api::Texture& shadows_texture);

   void build_acceleration_structures();

   void on_object_added_to_scene(const SceneObject& object);

 private:
   graphics_api::Device& m_device;
   resource::ResourceManager& m_resource_manager;
   render_objects::Mesh& m_teapot;
   Scene& m_scene;
   std::optional<graphics_api::Buffer> m_instance_list_buffer;
   graphics_api::Buffer m_object_buffer;
   graphics_api::ray_tracing::InstanceBuilder m_instance_builder;

   graphics_api::BufferHeap m_scratch_heap;
   graphics_api::ray_tracing::AccelerationStructurePool m_as_pool;
   graphics_api::ray_tracing::GeometryBuildContext m_build_blcontext;
   graphics_api::ray_tracing::GeometryBuildContext m_build_tlcontext;
   bool m_must_update_acceleration_structures{false};
   std::vector<ObjectDesc> m_objects;

   graphics_api::ray_tracing::AccelerationStructure* m_tl_acceleration_structure{};

   TG_SINK(Scene, OnObjectAddedToScene);
};

}// namespace triglav::renderer
