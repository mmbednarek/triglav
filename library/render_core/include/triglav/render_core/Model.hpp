#pragma once

#include <vector>

#include "graphics_api/HostVisibleBuffer.hpp"
#include "graphics_api/DescriptorArray.h"
#include "triglav/Name.hpp"

#include "Core.hpp"
#include "Material.hpp"

#include <geometry/Mesh.h>

namespace triglav::render_core {

struct MaterialRange
{
   size_t offset;
   size_t size;
   Name materialName;
};

struct Model
{
   graphics_api::Mesh<geometry::Vertex> mesh;
   geometry::BoundingBox boudingBox;
   std::vector<MaterialRange> range;
};

struct ModelShaderMapProperties
{
   graphics_api::UniformBuffer<ShadowMapUBO> ubo;
   graphics_api::DescriptorArray descriptors;
};

struct InstancedModel
{
   Name modelName;
   geometry::BoundingBox boudingBox;
   glm::vec3 position{};
   graphics_api::UniformBuffer<UniformBufferObject> ubo;
   graphics_api::UniformBuffer<MaterialProps> uboMatProps;
   graphics_api::DescriptorArray descriptors;
   ModelShaderMapProperties shadowMap;
};

struct Sprite
{
   float width{};
   float height{};
   graphics_api::UniformBuffer<SpriteUBO> ubo;
   graphics_api::DescriptorArray descriptors;
};

}// namespace renderer