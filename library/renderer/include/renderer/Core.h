#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include <stdexcept>

#include "graphics_api/Array.hpp"
#include "geometry/Geometry.h"

namespace renderer {

struct UniformBufferObject
{
   alignas(16) glm::mat4 model;
   alignas(16) glm::mat4 view;
   alignas(16) glm::mat4 proj;
};

struct MeshObject
{
   struct VertexIndex
   {
      uint32_t vertex;
      uint32_t uv;
      uint32_t normal;

      auto operator<=>(const VertexIndex& rhs) const = default;
   };

   std::vector<glm::vec3> vertices{};
   std::vector<glm::vec2> uvs{};
   std::vector<glm::vec3> normals{};
   std::vector<VertexIndex> indicies{};
};

struct Mesh
{
   std::vector<uint32_t> indicies{};
   std::vector<geometry::Vertex> vertices{};
};

using CompiledMesh = graphics_api::Mesh<geometry::Vertex>;

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
