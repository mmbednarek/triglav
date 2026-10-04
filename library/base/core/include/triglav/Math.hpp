#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <format>

#include "Int.hpp"
#include "MathConstants.hpp"
#include "Matrix.hpp"
#include "Name.hpp"
#include "Vector.hpp"

namespace triglav {

// Fake glm namespace
namespace glm {

using bvec2 = Vector2b;
using bvec3 = Vector3b;
using bvec4 = Vector4b;
using ivec2 = Vector2i;
using ivec3 = Vector3i;
using ivec4 = Vector4i;
using uvec2 = Vector2u;
using uvec3 = Vector3u;
using uvec4 = Vector4u;
using vec2 = Vector2;
using vec3 = Vector3;
using vec4 = Vector4;
using quat = Quaternion;
using mat3 = Matrix3x3;
using mat4 = Matrix4x4;

template<typename TVector>
TVector::ComponentType dot(const TVector& a, const TVector& b)
{
   return a.dot(b);
}

template<typename TVector>
TVector::ComponentType length(const TVector& a)
{
   return a.length();
}

template<typename TVector>
TVector cross(const TVector& a, const TVector& b)
{
   return a.cross(b);
}

template<typename TVector>
TVector normalize(const TVector& a)
{
   return a.normalize();
}

template<typename TMatrix>
TMatrix transpose(const TMatrix& mat)
{
   return mat.transpose();
}

template<typename TMatrix>
TMatrix inverse(const TMatrix& mat)
{
   return mat.inverse();
}

template<typename TVector>
TVector degrees(const TVector& vec)
{
   return vec.degrees();
}

template<typename TQuaternion>
Vector3 eulerAngles(const TQuaternion& quat)
{
   return quat.euler_angles();
}

template<typename TVector>
TVector::ComponentType distance(const TVector& a, const TVector& b)
{
   return (b - a).length();
}

}// namespace glm

// (x, y, width, height)
using Rect = Vector4;

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

[[nodiscard]] constexpr u32 divide_rounded_up(const u32 nominator, const u32 denominator)
{
   if ((nominator % denominator) == 0)
      return nominator / denominator;
   return 1 + (nominator / denominator);
}

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
   alignas(16) Quaternion rotation;
   alignas(16) Vector3 scale;
   alignas(16) Vector3 translation;

   static Transform3D identity();
   static Transform3D from_matrix(const Matrix4x4& matrix);
   static Transform3D null();
   [[nodiscard]] Matrix4x4 to_matrix() const;
   [[nodiscard]] Matrix4x4 to_normal_matrix() const;
   [[nodiscard]] Transform3D combine(const Transform3D& child) const;

   static Name meta_name()
   {
      return make_name_id("triglav::Transform3D");
   }
};

static_assert(sizeof(Transform3D) % 16 == 0);

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
struct std::formatter<triglav::Vector2>
{
   constexpr auto parse(std::format_parse_context& ctx)
   {
      return ctx.begin();
   }

   auto format(const triglav::Vector2& obj, std::format_context& ctx) const
   {
      return std::format_to(ctx.out(), "(X: {}, Y: {})", obj.x, obj.y);
   }
};

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
struct std::formatter<triglav::Vector4>
{
   constexpr auto parse(std::format_parse_context& ctx)
   {
      return ctx.begin();
   }

   auto format(const triglav::Vector4& obj, std::format_context& ctx) const
   {
      return std::format_to(ctx.out(), "(X: {}, Y: {}, Z: {}, W: {})", obj.x, obj.y, obj.z, obj.w);
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
      auto angles = obj.euler_angles().degrees();
      return std::format_to(ctx.out(), "(Yaw: {}, Pitch: {}, Roll: {})", angles.x, angles.y, angles.z);
   }
};

template<>
struct std::formatter<triglav::Matrix4x4>
{
   constexpr auto parse(std::format_parse_context& ctx)
   {
      return ctx.begin();
   }

   auto format(const triglav::Matrix4x4& obj, std::format_context& ctx) const
   {
      return std::format_to(ctx.out(), "({} {} {} {}; {} {} {} {}; {} {} {} {}; {} {} {} {})", obj[0].x, obj[0].y, obj[0].z, obj[0].w,
                            obj[1].x, obj[1].y, obj[1].z, obj[1].w, obj[2].x, obj[2].y, obj[2].z, obj[2].w, obj[3].x, obj[3].y, obj[3].z,
                            obj[3].w);
   }
};
