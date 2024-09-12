#pragma once

#include "triglav/graphics_api/ray_tracing/AccelerationStructurePool.hpp"
#include "triglav/graphics_api/ray_tracing/Geometry.hpp"
#include "triglav/geometry/Mesh.h"

#include <utility>

namespace triglav::renderer {

class RayTracingScene {
 public:
   explicit RayTracingScene(graphics_api::Device &device);

 private:
   graphics_api::Device &m_device;
   geometry::Mesh m_exampleMesh;
   std::optional<geometry::DeviceMesh> m_examplesMeshVertexData;
   graphics_api::Buffer m_boundingBoxBuffer;
   std::optional<graphics_api::Buffer> m_instanceListBuffer;

   graphics_api::ray_tracing::AccelerationStructurePool m_asPool;
   graphics_api::ray_tracing::GeometryBuildContext m_buildBLContext;
   graphics_api::ray_tracing::GeometryBuildContext m_buildTLContext;
};

}
