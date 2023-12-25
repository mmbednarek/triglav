#pragma once

#include <vector>

#include "graphics_api/HostVisibleBuffer.hpp"

#include "Core.h"
#include "Name.hpp"

namespace renderer {

struct MaterialRange
{
   size_t offset;
   size_t size;
   Name materialName;
};

struct Model
{
   Name meshName;
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
   graphics_api::UniformBuffer<UniformBufferObject> ubo;
   graphics_api::DescriptorArray descriptors;
   ModelShaderMapProperties shadowMap;
};

}// namespace renderer