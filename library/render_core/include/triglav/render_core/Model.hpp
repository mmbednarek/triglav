#pragma once

#include <vector>

#include "triglav/Name.hpp"
#include "triglav/geometry/Mesh.h"
#include "triglav/graphics_api/DescriptorArray.h"
#include "triglav/graphics_api/HostVisibleBuffer.hpp"

#include "Material.hpp"
#include "RenderCore.hpp"


namespace triglav::render_core {

struct MaterialRange
{
   size_t offset;
   size_t size;
   ResourceName materialName;
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
   graphics_api::UniformBuffer<ShadowMapUBO> ubo;
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
   float width{};
   float height{};
   graphics_api::UniformBuffer<SpriteUBO> ubo;
   graphics_api::DescriptorArray descriptors;
};

struct FragmentPushConstants
{
   alignas(16) glm::vec3 viewPosition;
};

}// namespace triglav::render_core