#pragma once

#include <glm/mat4x4.hpp>
#include <stdexcept>
#include <vector>

#include "geometry/Geometry.h"
#include "graphics_api/Array.hpp"

namespace renderer {

struct UniformBufferObject
{
   alignas(16) glm::mat4 model;
   alignas(16) glm::mat4 view;
   alignas(16) glm::mat4 proj;
   alignas(16) glm::mat4 normal;
   alignas(16) glm::mat4 shadowMapMVP;
};

struct SpriteUBO
{
   glm::mat3 model;
};

struct ShadowMapUBO
{
   alignas(16) glm::mat4 mvp;
};

using GpuMesh = graphics_api::Mesh<geometry::Vertex>;

template<typename TObject>
TObject checkResult(std::expected<TObject, graphics_api::Status> &&object)
{
   if (not object.has_value()) {
      throw std::runtime_error("failed to init graphics_api object");
   }
   return std::move(object.value());
}

inline void checkStatus(const graphics_api::Status status)
{
   if (status != graphics_api::Status::Success) {
      throw std::runtime_error("failed to init graphics_api object");
   }
}

}// namespace renderer
