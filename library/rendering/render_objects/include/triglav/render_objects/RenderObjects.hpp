#pragma once

#include "triglav/geometry/Geometry.hpp"

#include <glm/mat4x4.hpp>

namespace triglav::render_objects {

struct UniformBufferObject
{
   alignas(16) glm::mat4 model;
   alignas(16) glm::mat4 view;
   alignas(16) glm::mat4 proj;
   alignas(16) glm::mat4 normal;
   alignas(4) glm::vec3 viewPos;
};

struct SpriteUBO
{
   // 3x3 matrix needs to aligned by 4 floats
   // so we use 4x4 instead.
   glm::mat4 transform;
};

struct ShadowMapUBO
{
   alignas(16) glm::mat4 mvp;
};

using GpuMesh = graphics_api::Mesh<geometry::Vertex>;

}// namespace triglav::render_objects