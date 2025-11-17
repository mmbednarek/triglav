#pragma once

#include <vector>

#include "triglav/Name.hpp"
#include "triglav/geometry/Mesh.hpp"
#include "triglav/graphics_api/DescriptorArray.hpp"
#include "triglav/graphics_api/HostVisibleBuffer.hpp"

#include "Material.hpp"
#include "RenderObjects.hpp"

namespace triglav::render_objects {

struct Mesh
{
   graphics_api::Mesh<geometry::Vertex> mesh;
   geometry::BoundingBox bounding_box;
   std::vector<geometry::MaterialRange> range;
};

struct ModelShaderMapProperties
{
   ResourceName model_name;
   geometry::BoundingBox bounding_box;
   glm::mat4 model_mat;
   std::array<graphics_api::UniformBuffer<ShadowMapUBO>, 3> ubos;
};

struct InstancedModel
{
   ResourceName model_name;
   geometry::BoundingBox bounding_box;
   glm::vec3 position{};
   graphics_api::UniformBuffer<UniformBufferObject> ubo;
};

struct Sprite
{
   const graphics_api::Texture* texture;
   graphics_api::UniformBuffer<SpriteUBO> ubo;
};

}// namespace triglav::render_objects