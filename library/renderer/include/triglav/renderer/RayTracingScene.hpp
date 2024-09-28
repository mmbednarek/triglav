#pragma once

#include "Camera.hpp"

#include "triglav/geometry/Mesh.h"
#include "triglav/graphics_api/ReplicatedBuffer.hpp"
#include "triglav/graphics_api/ray_tracing/AccelerationStructurePool.hpp"
#include "triglav/graphics_api/ray_tracing/Geometry.hpp"
#include "triglav/graphics_api/ray_tracing/RayTracingPipeline.hpp"
#include "triglav/graphics_api/ray_tracing/ShaderBindingTable.hpp"

#include <glm/mat4x4.hpp>
#include <utility>

namespace triglav::resource {
class ResourceManager;
}

namespace triglav::renderer {

struct RayGenerationUboData {
   glm::mat4 viewInverse;
   glm::mat4 projInverse;
};
using RayGenerationUbo = graphics_api::UniformReplicatedBuffer<RayGenerationUboData>;

struct ObjectDesc {
   graphics_api::BufferAddress indexBuffer;
   graphics_api::BufferAddress vertexBuffer;
};

class RayTracingScene {
 public:
   explicit RayTracingScene(graphics_api::Device &device, resource::ResourceManager& resources);

   void render(graphics_api::CommandList& cmdList, const graphics_api::Texture& texture, Camera& camera);

 private:
   graphics_api::Device &m_device;
   geometry::Mesh m_exampleMeshBox;
   geometry::Mesh m_exampleMeshSphere;
   std::optional<geometry::DeviceMesh> m_examplesMeshBoxVertexData;
   std::optional<geometry::DeviceMesh> m_examplesMeshSphereVertexData;
   graphics_api::Buffer m_boundingBoxBuffer;
   std::optional<graphics_api::Buffer> m_instanceListBuffer;
   graphics_api::Buffer m_objectBuffer;

   graphics_api::ray_tracing::AccelerationStructurePool m_asPool;
   graphics_api::ray_tracing::GeometryBuildContext m_buildBLContext;
   graphics_api::ray_tracing::GeometryBuildContext m_buildTLContext;
   graphics_api::ray_tracing::RayTracingPipeline m_pipeline;
   graphics_api::ray_tracing::ShaderBindingTable m_bindingTable;
   RayGenerationUbo m_ubo;

   graphics_api::ray_tracing::AccelerationStructure* m_tlAccelerationStructure{};
};

}
