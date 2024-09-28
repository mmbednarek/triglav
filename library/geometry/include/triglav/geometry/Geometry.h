#pragma once

#include <cmath>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "triglav/graphics_api/Array.hpp"

namespace triglav::geometry {

using Index = uint32_t;

constexpr Index g_invalidIndex = std::numeric_limits<Index>::max();

constexpr bool is_valid(const Index index)
{
   return index != g_invalidIndex;
}

struct Vertex
{
   alignas(16) glm::vec3 location;
   glm::vec2 uv;
   alignas(16) glm::vec3 normal;
   alignas(16) glm::vec3 tangent;
   alignas(16) glm::vec3 bitangent;

   bool operator==(const Vertex& rhs) const = default;
};

struct IndexedVertex
{
   Index location;
   Index uv;
   Index normal;
   Index tangent;

   auto operator<=>(const IndexedVertex& rhs) const = default;
};

struct Extent3D
{
   float width;
   float height;
   float depth;
};

struct MeshGroup
{
   std::string name;
   std::string material;
};

struct MaterialRange
{
   size_t offset;
   size_t size;
   std::string materialName;
};

struct DeviceMesh
{
   graphics_api::Mesh<Vertex> mesh;
   std::vector<MaterialRange> ranges;
};

struct VertexData
{
   std::vector<Vertex> vertices;
   std::vector<u32> indices;
   std::vector<MaterialRange> ranges;
};

struct BoundingBox
{
   glm::vec3 min;
   glm::vec3 max;
};

constexpr double g_pi = 3.1415926535897932;

}// namespace triglav::geometry

namespace std {

template<>
struct hash<glm::vec3>
{
   size_t operator()(const glm::vec3 value) const noexcept
   {
      return static_cast<size_t>(3033917.0f * value.x + 5347961.0f * value.y + 5685221.0f * value.z);
   }
};

template<>
struct hash<glm::vec2>
{
   size_t operator()(const glm::vec2 value) const noexcept
   {
      return static_cast<size_t>(6878071.0f * value.x + 8562683.0f * value.y);
   }
};

template<>
struct hash<triglav::geometry::Vertex>
{
   size_t operator()(const triglav::geometry::Vertex& value) const noexcept
   {
      return 62327 * std::hash<glm::vec3>{}(value.location) + 36067 * std::hash<glm::vec3>{}(value.normal) +
             44381 * std::hash<glm::vec2>{}(value.uv) + 15937 * std::hash<glm::vec3>{}(value.tangent);
   }
};

}// namespace std