#pragma once

#include <vector>

#include "triglav/Name.hpp"
#include "triglav/geometry/Mesh.h"
#include "triglav/graphics_api/DescriptorArray.hpp"
#include "triglav/graphics_api/HostVisibleBuffer.hpp"

#include "Material.hpp"
#include "RenderCore.hpp"


namespace triglav::render_core {

struct MaterialRange
{
   size_t offset;
   size_t size;
   MaterialName materialName;
};

struct Model
{
   graphics_api::Mesh<geometry::Vertex> mesh;
   geometry::BoundingBox boundingBox;
   std::vector<MaterialRange> range;
};

struct ModelShaderMapProperties
{
   ResourceName modelName;
   geometry::BoundingBox boundingBox;
   glm::mat4 modelMat;
   std::array<graphics_api::UniformBuffer<ShadowMapUBO>, 3> ubos;
};

struct InstancedModel
{
   ResourceName modelName;
   geometry::BoundingBox boundingBox;
   glm::vec3 position{};
   graphics_api::UniformBuffer<UniformBufferObject> ubo;
};

struct Sprite
{
   const graphics_api::Texture* texture;
   graphics_api::UniformBuffer<SpriteUBO> ubo;
};

}// namespace triglav::render_core