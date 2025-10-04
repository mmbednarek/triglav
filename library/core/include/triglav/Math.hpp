#pragma once

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

// (r, g, b, a)
using Color = Vector4;

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

}// namespace triglav
