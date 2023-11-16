#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>
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

}// namespace renderer
