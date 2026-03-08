#pragma once

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"
#include "triglav/graphics_api/Array.hpp"

#include <cmath>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <optional>

namespace triglav::geometry {

using Index = uint32_t;

constexpr Index g_invalid_index = std::numeric_limits<Index>::max();

constexpr bool is_valid(const Index index)
{
   return index != g_invalid_index;
}

enum class VertexComponent : u32
{
   Core = (1 << 0),
   Texture = (1 << 1),
   NormalMap = (1 << 2),
   Skeleton = (1 << 3),
};

TRIGLAV_DECL_FLAGS(VertexComponent);

struct VertexComponentCore
{
   static constexpr VertexComponent Component = VertexComponent::Core;

   Vector3 location;
   Vector3 normal;

   bool operator==(const VertexComponentCore& rhs) const = default;
};

struct VertexComponentTexture
{
   static constexpr auto Component = VertexComponent::Texture;

   Vector2 uv;

   bool operator==(const VertexComponentTexture& rhs) const = default;
};

struct VertexComponentNormalMap
{
   static constexpr auto Component = VertexComponent::NormalMap;

   Vector4 tangent;

   bool operator==(const VertexComponentNormalMap& rhs) const = default;
};

struct VertexComponentSkeleton
{
   static constexpr auto Component = VertexComponent::Skeleton;

   Vector4i indices;
   Vector4 weights;

   bool operator==(const VertexComponentSkeleton& rhs) const = default;
};

[[nodiscard]] constexpr MemorySize get_vertex_size(const VertexComponentFlags components)
{
   MemorySize result{};
   if (components & VertexComponent::Core) {
      result += sizeof(VertexComponentCore);
   }
   if (components & VertexComponent::NormalMap) {
      result += sizeof(VertexComponentNormalMap);
   }
   if (components & VertexComponent::Texture) {
      result += sizeof(VertexComponentTexture);
   }
   if (components & VertexComponent::Skeleton) {
      result += sizeof(VertexComponentSkeleton);
   }
   return result;
}

constexpr VertexComponentFlags VERTEX_COMPONENTS_SIMPLE = VertexComponent::Core | VertexComponent::Texture;
constexpr VertexComponentFlags VERTEX_COMPONENTS_NORMAL_MAPPED =
   VertexComponent::Core | VertexComponent::Texture | VertexComponent::NormalMap;

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
   VertexComponentFlags components;
   MaterialName material;
};

struct VertexGroup
{
   VertexComponentFlags components;
   MaterialName material_name;
   size_t vertex_offset;
   size_t vertex_size;
   size_t index_offset;
   size_t index_size;
};

struct Ray
{
   Vector3 origin;
   Vector3 direction;
   float distance;
};

struct BoundingBox
{
   alignas(16) Vector3 min;
   alignas(16) Vector3 max;

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

   [[nodiscard]] BoundingBox transform(const Matrix4x4& mat) const
   {
      std::array<Vector4, 8> points{
         Vector4{min.x, min.y, min.z, 1.0f}, Vector4{min.x, min.y, max.z, 1.0f}, Vector4{min.x, max.y, min.z, 1.0f},
         Vector4{min.x, max.y, max.z, 1.0f}, Vector4{max.x, min.y, min.z, 1.0f}, Vector4{max.x, min.y, max.z, 1.0f},
         Vector4{max.x, max.y, min.z, 1.0f}, Vector4{max.x, max.y, max.z, 1.0f},
      };

      Vector3 bb_min{INFINITY, INFINITY, INFINITY};
      Vector3 bb_max{-INFINITY, -INFINITY, -INFINITY};
      for (const auto& p : points) {
         Vector4 r = mat * p;
         r /= r.w;

         bb_min = {
            std::min(bb_min.x, r.x),
            std::min(bb_min.y, r.y),
            std::min(bb_min.z, r.z),
         };
         bb_max = {
            std::max(bb_max.x, r.x),
            std::max(bb_max.y, r.y),
            std::max(bb_max.z, r.z),
         };
      }

      return BoundingBox{bb_min, bb_max};
   }

   [[nodiscard]] Vector3 scale() const
   {
      return max - min;
   }
};

// static_assert(sizeof(BoundingBox) % 16 == 0);

constexpr double g_pi = 3.1415926535897932;

}// namespace triglav::geometry

namespace std {

template<>
struct hash<triglav::Vector3>
{
   size_t operator()(const glm::vec3 value) const noexcept
   {
      return static_cast<size_t>(3033917.0f * value.x + 5347961.0f * value.y + 5685221.0f * value.z);
   }
};

template<>
struct hash<triglav::Vector2>
{
   size_t operator()(const glm::vec2 value) const noexcept
   {
      return static_cast<size_t>(6878071.0f * value.x + 8562683.0f * value.y);
   }
};

}// namespace std