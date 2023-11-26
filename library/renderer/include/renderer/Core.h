#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include <stdexcept>

#include "graphics_api/Buffer.h"

namespace renderer {

struct Vertex
{
   glm::vec3 position;
   glm::vec2 uv;
   glm::vec3 normal;
};

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
   std::vector<Vertex> vertices{};
};

struct CompiledMesh
{
   graphics_api::Buffer index_buffer;
   graphics_api::Buffer vertex_buffer;
   uint32_t index_count;
};


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
