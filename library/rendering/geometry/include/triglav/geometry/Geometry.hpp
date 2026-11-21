#pragma once

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"
#include "triglav/graphics_api/Array.hpp"
#include "triglav/meta/Meta.hpp"

#include <cmath>
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

   [[nodiscard]] constexpr Ray to_local_space(const Matrix4x4& inv_matrix) const
   {
      return Ray{
         .origin = (inv_matrix * Vector4{this->origin, 1.0f}).xyz(),
         .direction = (inv_matrix * Vector4{this->direction, 0.0f}).xyz(),
         .distance = this->distance,
      };
   }

   [[nodiscard]] constexpr std::optional<float> intersect_triangle(const std::array<Vector3, 3>& primitive) const
   {
      static constexpr float TOLERANCE = 0.0001f;

      const Vector3 edge1 = primitive[1] - primitive[0];
      const Vector3 edge2 = primitive[2] - primitive[0];

      const Vector3 h = this->direction.cross(edge2);
      const float a = edge1.dot(h);

      if (a > -TOLERANCE && a < TOLERANCE)
         return std::nullopt;

      const float f = 1.0f / a;
      const Vector3 s = this->origin - primitive[0];

      const float u = f * s.dot(h);
      if (u < 0.0f || u > 1.0f)
         return std::nullopt;

      const Vector3 q = s.cross(edge1);
      const float v = f * this->direction.dot(q);
      if (v < 0.0f || (v + u) > 1.0f)
         return std::nullopt;

      const auto t = f * edge2.dot(q);
      if (t < TOLERANCE)
         return std::nullopt;

      return t;
   }
};

struct SweepHit
{
   float time;
   Vector3 normal;

   [[nodiscard]] constexpr static SweepHit miss()
   {
      return {INF, {}};
   }

   [[nodiscard]] constexpr bool has_hit() const
   {
      return time < 1.0f;
   }
};

struct BoundingBox;
struct Capsule;

namespace detail {

struct SegmentBoxClosest
{
   float distance_sq{};
   Vector3 on_segment{};
   Vector3 on_box{};
};

[[nodiscard]] constexpr SegmentBoxClosest closest_segment_box(const Vector3& a, const Vector3& b, const BoundingBox& box);

// Closest point on triangle (a, b, c) to point p (Voronoi region test).
[[nodiscard]] constexpr Vector3 closest_point_triangle(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c)
{
   const Vector3 ab = b - a;
   const Vector3 ac = c - a;

   const Vector3 ap = p - a;
   const float d1 = ab.dot(ap);
   const float d2 = ac.dot(ap);
   if (d1 <= 0.0f && d2 <= 0.0f)
      return a;

   const Vector3 bp = p - b;
   const float d3 = ab.dot(bp);
   const float d4 = ac.dot(bp);
   if (d3 >= 0.0f && d4 <= d3)
      return b;

   const float vc = d1 * d4 - d3 * d2;
   if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
      return a + ab * (d1 / (d1 - d3));
   }

   const Vector3 cp = p - c;
   const float d5 = ab.dot(cp);
   const float d6 = ac.dot(cp);
   if (d6 >= 0.0f && d5 <= d6)
      return c;

   const float vb = d5 * d2 - d1 * d6;
   if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
      return a + ac * (d2 / (d2 - d6));
   }

   const float va = d3 * d6 - d5 * d4;
   if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
      return b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));
   }

   const float inv = 1.0f / (va + vb + vc);
   return a + ab * (vb * inv) + ac * (vc * inv);
}

// Closest points between segments [p1, q1] and [p2, q2].
struct SegmentSegmentClosest
{
   Vector3 on_first{};
   Vector3 on_second{};
};

[[nodiscard]] constexpr SegmentSegmentClosest closest_segment_segment(const Vector3& p1, const Vector3& q1, const Vector3& p2,
                                                                      const Vector3& q2)
{
   const Vector3 d1 = q1 - p1;
   const Vector3 d2 = q2 - p2;
   const Vector3 r = p1 - p2;
   const float a = d1.dot(d1);
   const float e = d2.dot(d2);
   const float f = d2.dot(r);

   float s = 0.0f;
   float t = 0.0f;
   if (a <= EPSILON && e <= EPSILON) {
      // Both degenerate to points
   } else if (a <= EPSILON) {
      t = clamp(f / e);
   } else {
      const float c = d1.dot(r);
      if (e <= EPSILON) {
         s = clamp(-c / a);
      } else {
         const float b = d1.dot(d2);
         const float denom = a * e - b * b;
         s = denom > EPSILON ? clamp((b * f - c * e) / denom) : 0.0f;
         t = (b * s + f) / e;
         if (t < 0.0f) {
            t = 0.0f;
            s = clamp(-c / a);
         } else if (t > 1.0f) {
            t = 1.0f;
            s = clamp((b - c) / a);
         }
      }
   }
   return {p1 + d1 * s, p2 + d2 * t};
}

struct SegmentTriangleClosest
{
   float distance_sq{};
   Vector3 on_segment{};
   Vector3 on_triangle{};
};

// Closest points between segment [p, q] and triangle (a, b, c).
// If the segment pierces the triangle the distance is 0. Otherwise the closest pair involves
// either a segment endpoint vs. the triangle, or the segment vs. one of the triangle's edges.
[[nodiscard]] constexpr SegmentTriangleClosest closest_segment_triangle(const Vector3& p, const Vector3& q, const Vector3& a,
                                                                        const Vector3& b, const Vector3& c)
{
   // 1. Segment piercing the triangle's interior
   const Vector3 normal = (b - a).cross(c - a);
   const Vector3 d = q - p;
   const float denom = normal.dot(d);
   if (std::abs(denom) > EPSILON) {
      const float t = normal.dot(a - p) / denom;
      if (t >= 0.0f && t <= 1.0f) {
         const Vector3 hit = p + d * t;
         const bool inside = (b - a).cross(hit - a).dot(normal) >= 0.0f && (c - b).cross(hit - b).dot(normal) >= 0.0f &&
                             (a - c).cross(hit - c).dot(normal) >= 0.0f;
         if (inside) {
            return {0.0f, hit, hit};
         }
      }
   }

   // 2. Best of: endpoints vs. triangle, segment vs. each edge
   SegmentTriangleClosest best{INF, {}, {}};
   const auto consider = [&](const Vector3& on_segment, const Vector3& on_triangle) {
      const Vector3 diff = on_segment - on_triangle;
      const float distance_sq = diff.dot(diff);
      if (distance_sq < best.distance_sq) {
         best = {distance_sq, on_segment, on_triangle};
      }
   };

   consider(p, closest_point_triangle(p, a, b, c));
   consider(q, closest_point_triangle(q, a, b, c));

   const Vector3* const vertices[3] = {&a, &b, &c};
   for (int i = 0; i < 3; ++i) {
      const SegmentSegmentClosest edge = closest_segment_segment(p, q, *vertices[i], *vertices[(i + 1) % 3]);
      consider(edge.on_first, edge.on_second);
   }
   return best;
}

}// namespace detail

struct Capsule
{
   TG_META_STRUCT_BODY(Capsule)

   Vector3 point_a{};
   Vector3 point_b{};
   float radius{};

   [[nodiscard]] constexpr Vector3 axis() const
   {
      return point_b - point_a;
   }

   [[nodiscard]] constexpr Vector3 center() const
   {
      return (point_a + point_b) * 0.5f;
   }

   [[nodiscard]] constexpr float height() const
   {
      return (point_b - point_a).length() + 2.0f * radius;
   }

   [[nodiscard]] constexpr Capsule transform(const Matrix4x4& trans) const
   {
      const float max_scale = std::max({
         trans[0].length(),
         trans[1].length(),
         trans[2].length(),
      });

      return {
         .point_a = (trans * Vector4{this->point_a, 1.0f}).xyz(),
         .point_b = (trans * Vector4{this->point_b, 1.0f}).xyz(),
         .radius = this->radius * max_scale,
      };
   }

   [[nodiscard]] constexpr Capsule extend_by_movement(const Vector3& movement) const
   {
      return {
         .point_a = this->point_a + 0.5f * movement,
         .point_b = this->point_b + 0.5f * movement,
         .radius = this->radius + movement.length(),
      };
   }

   [[nodiscard]] constexpr BoundingBox bounding_box() const;

   constexpr void translate(const Vector3& translation)
   {
      this->point_a += translation;
      this->point_b += translation;
   }

   [[nodiscard]] constexpr SweepHit sweep(const Capsule& capsule, const Vector3& movement) const
   {
      constexpr int MAX_ITERATIONS = 32;
      constexpr float TOLERANCE = 1e-4f;

      const float radius_sum = this->radius + capsule.radius;

      float time = 0.0f;
      for (int i = 0; i < MAX_ITERATIONS; ++i) {
         const Vector3 a = capsule.point_a + movement * time;
         const Vector3 b = capsule.point_b + movement * time;

         const detail::SegmentSegmentClosest closest = detail::closest_segment_segment(a, b, this->point_a, this->point_b);
         const Vector3 diff = closest.on_first - closest.on_second;
         const float distance = std::sqrt(diff.dot(diff));
         const float gap = distance - radius_sum;

         // Segments intersect: no meaningful normal from closest points.
         if (distance < EPSILON) {
            return {INF, {}};
         }

         const Vector3 normal = diff * (1.0f / distance);
         const float closing_speed = -movement.dot(normal);

         if (gap <= TOLERANCE) {
            // Touching or overlapping: hit only if actually moving toward the static capsule
            return closing_speed > EPSILON ? SweepHit{time, normal} : SweepHit::miss();
         }

         if (closing_speed <= EPSILON) {
            return {INF, {}};// Separating or moving parallel to the separating plane
         }

         time += gap / closing_speed;
         if (time > 1.0f) {
            return {INF, {}};
         }
      }

      return {INF, {}};// Did not converge (grazing); treat as a miss
   }
};

struct BoundingBox
{
   alignas(16) Vector3 min;
   alignas(16) Vector3 max;

   struct RayHit
   {
      float min_distance{INF};
      float max_distance{-INF};

      [[nodiscard]] static RayHit miss()
      {
         return {};
      }

      [[nodiscard]] bool has_hit() const
      {
         return min_distance != INF;
      }
   };

   struct SweepResult
   {
      float entry_time{INF};
      Vector3 normal{};

      [[nodiscard]] static SweepResult miss()
      {
         return {INF, {}};
      }

      [[nodiscard]] bool has_hit() const
      {
         return entry_time < 1.0f;
      }
   };

   [[nodiscard]] constexpr Vector3 centroid() const
   {
      return (max + min) * 0.5f;
   }

   [[nodiscard]] constexpr Vector3 half_extent() const
   {
      return (max - min) * 0.5f;
   }

   [[nodiscard]] constexpr RayHit intersect(const Ray& ray) const
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
         return RayHit{t_min, t_max};
      }
      return RayHit::miss();
   }

   [[nodiscard]] constexpr BoundingBox extend_by_movement(const Vector3& movement) const
   {
      BoundingBox bb = *this;
      for (const Axis axis : Vector3::AXES) {
         if (movement[axis] > 0.0f) {
            bb.max[axis] += movement[axis];
         } else {
            bb.min[axis] += movement[axis];
         }
      }
      return bb;
   }

   [[nodiscard]] constexpr SweepResult sweep_aabb(const BoundingBox& dynamic_box, const Vector3& movement) const
   {
      Vector3 inv_entry;
      Vector3 inv_exit;
      for (const Axis axis : Vector3::AXES) {
         if (movement[axis] > 0.0f) {
            inv_entry[axis] = this->min[axis] - dynamic_box.max[axis];
            inv_exit[axis] = this->max[axis] - dynamic_box.min[axis];
         } else {
            inv_entry[axis] = this->max[axis] - dynamic_box.min[axis];
            inv_exit[axis] = this->min[axis] - dynamic_box.max[axis];
         }
      }

      Vector3 entry{-INF};
      Vector3 exit{INF};
      for (const Axis axis : Vector3::AXES) {
         if (std::abs(movement[axis]) < EPSILON) {
            if (dynamic_box.max[axis] < this->min[axis] || dynamic_box.min[axis] > this->max[axis]) {
               return {INF, {}};
            }
            continue;
         }

         entry[axis] = inv_entry[axis] / movement[axis];
         exit[axis] = inv_exit[axis] / movement[axis];
      }

      const Axis entry_axis = entry.major_axis();
      const float entry_time = entry[entry_axis];
      const float exit_time = exit[exit.minor_axis()];

      Vector3 normal;
      normal[entry_axis] = inv_entry[entry_axis] > 0.0f ? -1.0f : 1.0f;

      if (entry_time > exit_time || entry_time > 1.0f || exit_time < 0.0f) {
         return {INF, {}};
      }

      return {entry_time, normal};
   }

   [[nodiscard]] Vector3 minimum_translation_vector(const BoundingBox& dynamic_bb) const
   {
      if (!this->does_intersect_aabb(dynamic_bb))
         return {};

      const Vector3 push{
         (dynamic_bb.min.x < this->min.x) ? (this->min.x - dynamic_bb.max.x) : (this->max.x - dynamic_bb.min.x),
         (dynamic_bb.min.y < this->min.y) ? (this->min.y - dynamic_bb.max.y) : (this->max.y - dynamic_bb.min.y),
         (dynamic_bb.min.z < this->min.z) ? (this->min.z - dynamic_bb.max.z) : (this->max.z - dynamic_bb.min.z),
      };
      const Vector3 abs = push.abs();

      const Axis min_axis = abs.minor_axis();

      Vector3 result{};
      result[min_axis] = push[min_axis];
      return result;
   }

   [[nodiscard]] constexpr bool does_intersect_aabb(const BoundingBox& bb) const
   {
      return (this->max - bb.min).all(is_greater_or_equal<0.0f>) && (bb.max - this->min).all(is_greater_or_equal<0.0f>);
   }

   [[nodiscard]] constexpr bool does_intersect_capsule(const Capsule& capsule) const
   {
      const auto closest = detail::closest_segment_box(capsule.point_a, capsule.point_b, *this);
      return closest.distance_sq <= (capsule.radius * capsule.radius);
   }

   [[nodiscard]] constexpr bool does_intersect(const Ray& ray) const
   {
      return this->intersect(ray).has_hit();
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

   [[nodiscard]] bool operator!=(const BoundingBox& rhs) const noexcept
   {
      return this->min != rhs.min || this->max != rhs.max;
   }

   [[nodiscard]] constexpr Axis major_axis() const
   {
      return this->scale().major_axis();
   }

   constexpr void translate(const Vector3& vector)
   {
      this->min += vector;
      this->max += vector;
   }

   [[nodiscard]] BoundingBox merge(const BoundingBox& right) const
   {
      return {
         .min = {std::min(this->min.x, right.min.x), std::min(this->min.y, right.min.y), std::min(this->min.z, right.min.z)},
         .max = {std::max(this->max.x, right.max.x), std::max(this->max.y, right.max.y), std::max(this->max.z, right.max.z)},
      };
   }

   [[nodiscard]] constexpr float half_surface_area() const
   {
      const auto scale = this->scale();
      return scale.x * scale.y + scale.x * scale.z + scale.y * scale.z;
   }

   [[nodiscard]] constexpr float extension_cost(const BoundingBox& extension) const
   {
      return this->merge(extension).half_surface_area() - this->half_surface_area();
   }

   [[nodiscard]] constexpr SweepResult sweep_capsule(const Capsule& dynamic_capsule, const Vector3& movement) const
   {
      constexpr int MAX_ITERATIONS = 32;
      constexpr float TOLERANCE = 1e-4f;

      float time = 0.0f;
      for (int i = 0; i < MAX_ITERATIONS; ++i) {
         const Vector3 a = dynamic_capsule.point_a + movement * time;
         const Vector3 b = dynamic_capsule.point_b + movement * time;

         const detail::SegmentBoxClosest closest = detail::closest_segment_box(a, b, *this);
         const float distance = std::sqrt(closest.distance_sq);
         const float gap = distance - dynamic_capsule.radius;

         // Segment is inside the box: no meaningful normal from closest points.
         if (distance < EPSILON) {
            return SweepResult::miss();
         }

         const Vector3 normal = (closest.on_segment - closest.on_box) * (1.0f / distance);
         const float closing_speed = -movement.dot(normal);

         if (gap <= TOLERANCE) {
            // Touching or overlapping: hit only if actually moving into the box
            return closing_speed > EPSILON ? SweepResult{time, normal} : SweepResult::miss();
         }

         if (closing_speed <= EPSILON) {
            return SweepResult::miss();// Separating or moving parallel to the separating plane
         }

         time += gap / closing_speed;
         if (time > 1.0f) {
            return SweepResult::miss();
         }
      }

      return SweepResult::miss();// Did not converge (grazing); treat as a miss
   }
};

[[nodiscard]] constexpr BoundingBox Capsule::bounding_box() const
{
   return {
      .min =
         {
            std::min(point_a.x - radius, point_b.x - radius),
            std::min(point_a.y - radius, point_b.y - radius),
            std::min(point_a.z - radius, point_b.z - radius),
         },
      .max =
         {
            std::max(point_a.x + radius, point_b.x + radius),
            std::max(point_a.y + radius, point_b.y + radius),
            std::max(point_a.z + radius, point_b.z + radius),
         },
   };
}

namespace detail {

// Exact closest points between segment [a, b] and a box.
// f(t) = |p(t) - clamp(p(t), box)|^2 is convex and piecewise quadratic. Between breakpoints
// (where p(t) crosses a slab plane) the clamp state per axis is constant, so each piece is
// minimised in closed form.
[[nodiscard]] constexpr SegmentBoxClosest closest_segment_box(const Vector3& a, const Vector3& b, const BoundingBox& box)
{
   const Vector3 d = b - a;

   float ts[8];
   int count = 0;
   ts[count++] = 0.0f;
   ts[count++] = 1.0f;
   for (const Axis axis : Vector3::AXES) {
      if (std::abs(d[axis]) < EPSILON) {
         continue;
      }
      const float t_min = (box.min[axis] - a[axis]) / d[axis];
      const float t_max = (box.max[axis] - a[axis]) / d[axis];
      if (t_min > 0.0f && t_min < 1.0f)
         ts[count++] = t_min;
      if (t_max > 0.0f && t_max < 1.0f)
         ts[count++] = t_max;
   }

   for (int i = 1; i < count; ++i) {
      const float key = ts[i];
      int j = i - 1;
      while (j >= 0 && ts[j] > key) {
         ts[j + 1] = ts[j];
         --j;
      }
      ts[j + 1] = key;
   }

   const auto evaluate = [&](const float t) {
      SegmentBoxClosest result;
      for (const Axis axis : Vector3::AXES) {
         const float p = a[axis] + d[axis] * t;
         const float q = p < box.min[axis] ? box.min[axis] : (p > box.max[axis] ? box.max[axis] : p);
         result.on_segment[axis] = p;
         result.on_box[axis] = q;
         result.distance_sq += (p - q) * (p - q);
      }
      return result;
   };

   SegmentBoxClosest best = evaluate(0.0f);
   for (int i = 0; i + 1 < count; ++i) {
      const float t0 = ts[i];
      const float t1 = ts[i + 1];
      const float mid = 0.5f * (t0 + t1);

      float numerator = 0.0f;
      float denominator = 0.0f;
      for (const Axis axis : Vector3::AXES) {
         const float p = a[axis] + d[axis] * mid;
         float bound;
         if (p < box.min[axis]) {
            bound = box.min[axis];
         } else if (p > box.max[axis]) {
            bound = box.max[axis];
         } else {
            continue;
         }
         numerator += d[axis] * (a[axis] - bound);
         denominator += d[axis] * d[axis];
      }

      float t = t0;
      if (denominator > 0.0f) {
         t = -numerator / denominator;
         t = t < t0 ? t0 : (t > t1 ? t1 : t);
      }

      const SegmentBoxClosest candidate = evaluate(t);
      if (candidate.distance_sq < best.distance_sq) {
         best = candidate;
      }
   }
   return best;
}
}// namespace detail

// static_assert(sizeof(BoundingBox) % 16 == 0);

constexpr double g_pi = PI;

class ITraversable
{
 public:
   virtual ~ITraversable() = default;

   [[nodiscard]] virtual SweepHit generic_sweep_cast(const BoundingBox& box, const Vector3& movement) const = 0;
   [[nodiscard]] virtual float generic_ray_cast(const Ray& ray) const = 0;
};

[[nodiscard]] constexpr SweepHit sweep_triangle_aabb(const std::array<Vector3, 3>& triangle, const BoundingBox& bb, const Vector3& movement)
{
   constexpr static float TOLERANCE = 1e-8f;

   // Work in box space: box centered at origin, triangle static.
   const auto box_center = bb.centroid();
   const Vector3 v[3] = {triangle[0] - box_center, triangle[1] - box_center, triangle[2] - box_center};
   const Vector3 h = bb.half_extent();

   const Vector3 e[3] = {v[1] - v[0], v[2] - v[1], v[0] - v[2]};
   static constexpr std::array box_axes = {Vector3{1, 0, 0}, Vector3{0, 1, 0}, Vector3{0, 0, 1}};

   // Build the 13 candidate axes.
   Vector3 axes[13];
   int n_axes = 0;
   for (int i = 0; i < 3; ++i)
      axes[n_axes++] = box_axes[i];
   axes[n_axes++] = e[0].cross(e[1]);// triangle normal
   for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j)
         axes[n_axes++] = box_axes[i].cross(e[j]);// edge x edge

   float t_enter = -INF;// latest entry over all axes
   float t_exit = INF;  // earliest exit over all axes
   Vector3 enter_normal{};

   // Fallback for initial overlap: axis with smallest penetration.
   float best_depth = INF;
   Vector3 mtv_normal{};

   for (int i = 0; i < n_axes; ++i) {
      if (axes[i].length_sq() < TOLERANCE)// degenerate (parallel edges, ...)
         continue;
      const Vector3 L = axes[i].normalize();

      // Box radius on L, triangle interval on L, box velocity on L.
      const float r = h.x * std::fabs(L.x) + h.y * std::fabs(L.y) + h.z * std::fabs(L.z);
      const float d0 = v[0].dot(L), d1 = v[1].dot(L), d2 = v[2].dot(L);
      const float tmin = std::min({d0, d1, d2});
      const float tmax = std::max({d0, d1, d2});
      const float s = movement.dot(L);

      // Initial penetration depth on this axis (push box toward -L or +L).
      const float push_neg = r - tmin;// box_max - tri_min
      const float push_pos = tmax + r;// tri_max - box_min
      if (push_neg < 0.0f || push_pos < 0.0f) {
         // Separated at t = 0 on this axis.
      } else {
         const float depth = std::min(push_neg, push_pos);
         if (depth < best_depth) {
            best_depth = depth;
            mtv_normal = (push_neg < push_pos) ? L * -1.0f : L;
         }
      }

      if (std::fabs(s) < TOLERANCE) {
         // No relative motion on this axis: permanently separated or overlapping.
         if (tmin > r || tmax < -r)
            return SweepHit::miss();
         continue;
      }

      float enter, exit_;
      Vector3 n;
      if (s > 0.0f) {// box approaches from the -L side
         enter = (tmin - r) / s;
         exit_ = (tmax + r) / s;
         n = L * -1.0f;
      } else {// box approaches from the +L side
         enter = (tmax + r) / s;
         exit_ = (tmin - r) / s;
         n = L;
      }

      if (enter > t_enter) {
         t_enter = enter;
         enter_normal = n;
      }
      t_exit = std::min(t_exit, exit_);

      if (t_enter > t_exit || t_exit < 0.0f || t_enter >= 1.0f)
         return SweepHit::miss();
   }

   if (t_enter < 0.0f) {
      // Overlapping at t = 0 on every axis: report an immediate hit with the
      // minimum-translation normal, but only if we're not already leaving.
      if (best_depth == INF)
         return SweepHit::miss();
      if (movement.dot(mtv_normal) >= 0.0f)
         return SweepHit::miss();

      return {0.0f, mtv_normal};
   }

   return {t_enter, enter_normal};
}

[[nodiscard]] constexpr SweepHit sweep_triangle_capsule(const std::array<Vector3, 3>& triangle, const Capsule& dynamic_capsule,
                                                        const Vector3& movement)
{
   constexpr int MAX_ITERATIONS = 32;
   constexpr float TOLERANCE = 1e-4f;

   float time = 0.0f;
   for (int i = 0; i < MAX_ITERATIONS; ++i) {
      const Vector3 a = dynamic_capsule.point_a + movement * time;
      const Vector3 b = dynamic_capsule.point_b + movement * time;

      const detail::SegmentTriangleClosest closest = detail::closest_segment_triangle(a, b, triangle[0], triangle[1], triangle[2]);
      const float distance = std::sqrt(closest.distance_sq);
      const float gap = distance - dynamic_capsule.radius;

      // Segment touches or pierces the triangle: no meaningful normal from closest points.
      if (distance < EPSILON) {
         return SweepHit::miss();
      }

      const Vector3 normal = (closest.on_segment - closest.on_triangle) * (1.0f / distance);
      const float closing_speed = -movement.dot(normal);

      if (gap <= TOLERANCE) {
         return closing_speed > EPSILON ? SweepHit{time, normal} : SweepHit{INF, {}};
      }

      if (closing_speed <= EPSILON) {
         return SweepHit::miss();
      }

      time += gap / closing_speed;
      if (time > 1.0f) {
         return SweepHit::miss();
      }
   }

   return SweepHit::miss();
}

}// namespace triglav::geometry

namespace std {

template<>
struct hash<triglav::Vector3>
{
   size_t operator()(const triglav::Vector3 value) const noexcept
   {
      return static_cast<size_t>(3033917.0f * value.x + 5347961.0f * value.y + 5685221.0f * value.z);
   }
};

template<>
struct hash<triglav::Vector2>
{
   size_t operator()(const triglav::Vector2 value) const noexcept
   {
      return static_cast<size_t>(6878071.0f * value.x + 8562683.0f * value.y);
   }
};

}// namespace std