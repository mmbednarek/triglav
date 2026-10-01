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

struct Quaternion
{
   float x{}, y{}, z{}, w{};

   constexpr Quaternion() = default;

   constexpr Quaternion(const float w, const float x, const float y, const float z) :
       x(x),
       y(y),
       z(z),
       w(w)
   {
   }

   [[nodiscard]] static Quaternion identity()
   {
      return {1.0f, 0.0f, 0.0f, 0.0f};
   }

   [[nodiscard]] constexpr Vector3 euler_angles() const noexcept
   {
      // Roll (x-axis rotation)
      const float sinr_cosp = 2.0f * (w * x + y * z);
      const float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
      const float out_x = std::atan2(sinr_cosp, cosr_cosp);

      // Pitch (y-axis rotation)
      const float sinp = 2.0f * (w * y - z * x);
      const float out_y = std::asin(std::clamp(sinp, -1.0f, 1.0f));

      // Yaw (z-axis rotation)
      const float siny_cosp = 2.0f * (w * z + x * y);
      const float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
      const float out_z = std::atan2(siny_cosp, cosy_cosp);

      return {out_x, out_y, out_z};
   }

   [[nodiscard]] constexpr static Quaternion from_euler_angles(const Vector3& euler)
   {
      // Half angles in radians
      const float half_x = euler.x * 0.5f;// Roll
      const float half_y = euler.y * 0.5f;// Pitch
      const float half_z = euler.z * 0.5f;// Yaw

      const float cx = std::cos(half_x);
      const float sx = std::sin(half_x);
      const float cy = std::cos(half_y);
      const float sy = std::sin(half_y);
      const float cz = std::cos(half_z);
      const float sz = std::sin(half_z);

      return Quaternion{
         cz * cy * cx + sz * sy * sx,// w
         cz * cy * sx - sz * sy * cx,// x
         cz * sy * cx + sz * cy * sx,// y
         sz * cy * cx - cz * sy * sx // z
      };
   }

   [[nodiscard]] constexpr bool operator==(const Quaternion& rhs) const noexcept
   {
      return this->w == rhs.w && this->x == rhs.x && this->y == rhs.y && this->z == rhs.z;
   }

   [[nodiscard]] constexpr Quaternion operator*(const Quaternion& rhs) const noexcept
   {
      return Quaternion{w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z, w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
                        w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x, w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w};
   }

   [[nodiscard]] constexpr Quaternion operator/(const float constant) const noexcept
   {
      return Quaternion{w / constant, x / constant, y / constant, z / constant};
   }

   [[nodiscard]] constexpr Vector3 operator*(const Vector3& rhs) const noexcept
   {
      const Vector3 t{
         2.0f * (y * rhs.z - z * rhs.y),
         2.0f * (z * rhs.x - x * rhs.z),
         2.0f * (x * rhs.y - y * rhs.x),
      };

      return Vector3{
         rhs.x + w * t.x + (y * t.z - z * t.y),
         rhs.y + w * t.y + (z * t.x - x * t.z),
         rhs.z + w * t.z + (x * t.y - y * t.x),
      };
   }

   constexpr Quaternion& operator*=(const Quaternion& rhs) noexcept
   {
      *this = *this * rhs;
      return *this;
   }

   constexpr Quaternion& operator*=(const float mult) noexcept
   {
      this->w *= mult;
      this->x *= mult;
      this->y *= mult;
      this->z *= mult;
      return *this;
   }

   [[nodiscard]] constexpr Quaternion normalize() const
   {
      const auto length = std::sqrt(w * w * +x * x + y * y + z * z);
      return Quaternion{w / length, x / length, y / length, z / length};
   }

   [[nodiscard]] static Quaternion from_rotation_matrix(const Matrix3x3& r) noexcept
   {
      Quaternion q{};
      const float trace = r[0][0] + r[1][1] + r[2][2];

      if (trace > 0.0f) {
         // w is the largest component
         const float s = std::sqrt(trace + 1.0f) * 2.0f;// s = 4 * w
         q.w = 0.25f * s;
         q.x = (r[2][1] - r[1][2]) / s;
         q.y = (r[0][2] - r[2][0]) / s;
         q.z = (r[1][0] - r[0][1]) / s;
      } else if ((r[0][0] > r[1][1]) && (r[0][0] > r[2][2])) {
         // x is the largest component
         const float s = std::sqrt(1.0f + r[0][0] - r[1][1] - r[2][2]) * 2.0f;// s = 4 * x
         q.w = (r[2][1] - r[1][2]) / s;
         q.x = 0.25f * s;
         q.y = (r[0][1] + r[1][0]) / s;
         q.z = (r[0][2] + r[2][0]) / s;
      } else if (r[1][1] > r[2][2]) {
         // y is the largest component
         const float s = std::sqrt(1.0f + r[1][1] - r[0][0] - r[2][2]) * 2.0f;// s = 4 * y
         q.w = (r[0][2] - r[2][0]) / s;
         q.x = (r[0][1] + r[1][0]) / s;
         q.y = 0.25f * s;
         q.z = (r[1][2] + r[2][1]) / s;
      } else {
         // z is the largest component
         const float s = std::sqrt(1.0f + r[2][2] - r[0][0] - r[1][1]) * 2.0f;// s = 4 * z
         q.w = (r[1][0] - r[0][1]) / s;
         q.x = (r[0][2] + r[2][0]) / s;
         q.y = (r[1][2] + r[2][1]) / s;
         q.z = 0.25f * s;
      }

      return q;
   }

   static Quaternion angle_axis(const float angle, const Vector3& v)
   {
      const Vector3 vs = v * std::sin(angle * 0.5f);
      return Quaternion{std::cos(angle * 0.5f), vs.x, vs.y, vs.z};
   }

   [[nodiscard]] static Quaternion from_oriented_vector(const Vector3& source, const Vector3& target) noexcept
   {
      const float cos_theta = source.dot(target);
      Vector3 rotation_axis;

      if (cos_theta >= 1.0f - EPSILON) {
         return identity();
      }

      if (cos_theta < -1.0f + EPSILON) {
         rotation_axis = Vector3(0, 0, 1).cross(source);
         if (rotation_axis.length() < EPSILON)// bad luck, they were parallel, try again!
            rotation_axis = Vector3(1, 0, 0).cross(source);

         rotation_axis = rotation_axis.normalize();
         return angle_axis(PI, rotation_axis);
      }

      rotation_axis = source.cross(target);

      const float s = std::sqrt((1.0f + cos_theta) * 2.0f);
      const float invs = 1.0f / s;

      return Quaternion(s * 0.5f, rotation_axis.x * invs, rotation_axis.y * invs, rotation_axis.z * invs);
   }

   [[nodiscard]] constexpr Quaternion inverse() const noexcept
   {
      const float n2 = x * x + y * y + z * z + w * w;

      // Handle division by zero for zero-quaternions
      if (n2 < 1e-8f) {
         return {0.0, 0.0, 0.0, 0.0};
      }

      const float inv_n2 = 1.0f / n2;
      return {-x * inv_n2, -y * inv_n2, -z * inv_n2, w * inv_n2};
   }
};

constexpr Matrix3x3 Matrix3x3::rotation(const Quaternion& q)
{
   Matrix3x3 result = identity();
   const float qxx = q.x * q.x;
   const float qyy = q.y * q.y;
   const float qzz = q.z * q.z;
   const float qxz = q.x * q.z;
   const float qxy = q.x * q.y;
   const float qyz = q.y * q.z;
   const float qwx = q.w * q.x;
   const float qwy = q.w * q.y;
   const float qwz = q.w * q.z;

   result[0][0] = 1.0f - 2.0f * (qyy + qzz);
   result[0][1] = 2.0f * (qxy + qwz);
   result[0][2] = 2.0f * (qxz - qwy);

   result[1][0] = 2.0f * (qxy - qwz);
   result[1][1] = 1.0f - 2.0f * (qxx + qzz);
   result[1][2] = 2.0f * (qyz + qwx);

   result[2][0] = 2.0f * (qxz + qwy);
   result[2][1] = 2.0f * (qyz - qwx);
   result[2][2] = 1.0f - 2.0f * (qxx + qyy);

   return result;
}

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
      return {0, 0, 1};
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

[[nodiscard]] constexpr Axis vector3_min_axis(const Vector3& vec)
{
   if (vec.x < vec.y) {
      if (vec.x < vec.z) {
         return Axis::X;
      }
      if (vec.z < vec.y) {
         return Axis::Z;
      }
   } else if (vec.z < vec.y) {
      return Axis::Z;
   }
   return Axis::Y;
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
