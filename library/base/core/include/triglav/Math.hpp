#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <format>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Int.hpp"

namespace triglav {

using Vector2i = glm::ivec2;
using Vector3i = glm::ivec3;
using Vector4i = glm::ivec4;
using Vector2u = glm::uvec2;
using Vector3u = glm::uvec3;
using Vector4u = glm::uvec4;

using Vector2 = glm::vec2;
using Vector3 = glm::vec3;
using Vector4 = glm::vec4;
using Quaternion = glm::quat;

// (x, y, width, height)
using Rect = Vector4;

enum class Axis : u32
{
   X = 0,
   Y = 1,
   Z = 2,
   W = 3
};

[[nodiscard]] constexpr Vector3 axis_forward_vec3(const Axis axis)
{
   switch (axis) {
   case Axis::X:
      return {1, 0, 0};
   case Axis::Y:
      return {0, 1, 0};
   case Axis::Z:
      return {0, 0, 01};
   default:
      return {0, 0, 0};
   }
}

[[nodiscard]] constexpr float& vector3_component(Vector3& vec, const Axis axis)
{
   static float def_res = 0.0f;
   switch (axis) {
   case Axis::X:
      return vec.x;
   case Axis::Y:
      return vec.y;
   case Axis::Z:
      return vec.z;
   default:
      return def_res;
   }
}

[[nodiscard]] constexpr float vector3_component(const Vector3& vec, const Axis axis)
{
   return std::bit_cast<std::array<float, 3>>(vec)[static_cast<u32>(axis)];
}


[[nodiscard]] constexpr Vector2 rect_position(const Rect& r)
{
   return {r.x, r.y};
}

[[nodiscard]] constexpr Vector2 rect_size(const Rect& r)
{
   return {r.z, r.w};
}

// (r, g, b, a)
using Color = Vector4;

namespace palette {

constexpr auto BLACK = Color{0, 0, 0, 1.0f};
constexpr auto WHITE = Color{1.0f, 1.0f, 1.0f, 1.0f};
constexpr auto NO_COLOR = Color{0.0f, 0.0f, 0.0f, 0.0f};

}// namespace palette

struct Vector3_Aligned16B
{
   alignas(16) Vector3 value{};
};

using Matrix3x3 = glm::mat3x3;
using Matrix4x4 = glm::mat4x4;

[[nodiscard]] constexpr u32 divide_rounded_up(const u32 nominator, const u32 denominator)
{
   if ((nominator % denominator) == 0)
      return nominator / denominator;
   return 1 + (nominator / denominator);
}

constexpr auto g_pi = 3.14159265358979323846f;

[[nodiscard]] constexpr float lerp(const float a, const float b, const float t)
{
   return a + (b - a) * t;
}

[[nodiscard]] constexpr float sign(const float a)
{
   return a > 0.0f ? 1.0f : -1.0f;
}

[[nodiscard]] inline bool do_regions_intersect(const Vector4 a, const Vector4 b)
{
   return a.x <= (b.x + b.z) && (a.x + a.z) >= b.x && a.y <= (b.y + b.w) && (a.y + a.w) >= b.y;
}

[[nodiscard]] inline bool is_point_inside(const Rect area, const Vector2 point)
{
   return point.x >= area.x && point.y >= area.y && point.x < (area.x + area.z) && point.y < (area.y + area.w);
}

[[nodiscard]] inline Vector4 min_area(const Vector4 lhs, const Vector4 rhs)
{
   return {std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y), std::min(lhs.z, rhs.z), std::min(lhs.w, rhs.w)};
}

struct Transform3D
{
   Quaternion rotation;
   Vector3 scale;
   Vector3 translation;

   static Transform3D identity();
   static Transform3D from_matrix(const Matrix4x4& matrix);
   [[nodiscard]] Matrix4x4 to_matrix() const;
};

[[nodiscard]] constexpr MemorySize align_size(const MemorySize size, const MemorySize alignment)
{
   const auto offset = size % alignment;
   if (offset == 0)
      return size;
   return size - offset + alignment;
}

[[nodiscard]] Vector3 find_closest_point_between_lines(Vector3 origin_a, Vector3 dir_a, Vector3 origin_b, Vector3 dir_b);
[[nodiscard]] Vector3 find_closest_point_on_line(Vector3 origin, Vector3 dir, Vector3 point);
[[nodiscard]] Vector3 find_point_on_aa_surface(Vector3 origin, Vector3 dir, Axis axis_surface, float surface);

}// namespace triglav

template<>
struct std::formatter<triglav::Vector3>
{
   constexpr auto parse(std::format_parse_context& ctx)
   {
      return ctx.begin();
   }

   auto format(const triglav::Vector3& obj, std::format_context& ctx) const
   {
      return std::format_to(ctx.out(), "(X: {}, Y: {}, Z: {})", obj.x, obj.y, obj.z);
   }
};
template<>
struct std::formatter<triglav::Quaternion>
{
   constexpr auto parse(std::format_parse_context& ctx)
   {
      return ctx.begin();
   }

   auto format(const triglav::Quaternion& obj, std::format_context& ctx) const
   {
      auto angles = glm::degrees(glm::eulerAngles(obj));
      return std::format_to(ctx.out(), "(Yaw: {}, Pitch: {}, Roll: {})", angles.x, angles.y, angles.z);
   }
};
