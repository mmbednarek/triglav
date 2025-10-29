#pragma once

#include "triglav/Name.hpp"
#include "triglav/graphics_api/Array.hpp"

#include <cmath>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <optional>

namespace triglav::geometry {

using Index = uint32_t;

constexpr Index g_invalidIndex = std::numeric_limits<Index>::max();

constexpr bool is_valid(const Index index)
{
   return index != g_invalidIndex;
}

struct Vertex
{
   glm::vec3 location;
   glm::vec2 uv;
   glm::vec3 normal;
   glm::vec4 tangent;

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
   MaterialName material;
};

struct MaterialRange
{
   size_t offset;
   size_t size;
   MaterialName materialName;
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

struct Ray
{
   Vector3 origin;
   Vector3 direction;
   float distance;
};

struct BoundingBox
{
   Vector3 min;
   Vector3 max;

   [[nodiscard]] constexpr Vector3 centroid() const
   {
      return (max + min) * 0.5f;
   }

   [[nodiscard]] constexpr std::optional<Vector2> intersect(const Ray& ray) const
   {
      const float tx1 = (min.x - ray.origin.x) / ray.direction.x;
      const float tx2 = (max.x - ray.origin.x) / ray.direction.x;
      float t_min = std::min(tx1, tx2);
      float t_max = std::max(tx1, tx2);
      const float ty1 = (min.y - ray.origin.y) / ray.direction.y;
      const float ty2 = (max.y - ray.origin.y) / ray.direction.y;
      t_min = std::max(t_min, std::min(ty1, ty2));
      t_max = std::min(t_max, std::max(ty1, ty2));
      const float tz1 = (min.z - ray.origin.z) / ray.direction.z;
      const float tz2 = (max.z - ray.origin.z) / ray.direction.z;
      t_min = std::max(t_min, std::min(tz1, tz2));
      t_max = std::min(t_max, std::max(tz1, tz2));
      if (t_max >= t_min && t_min < ray.distance && t_max > 0) {
         return Vector2{t_min, t_max};
      }
      return std::nullopt;
   }

   [[nodiscard]] constexpr bool does_intersect(const Ray& ray) const
   {
      return this->intersect(ray).has_value();
   }

   [[nodiscard]] Vector3 scale() const
   {
      return max - min;
   }
};

struct MeshData
{
   VertexData vertexData;
   BoundingBox boundingBox;
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