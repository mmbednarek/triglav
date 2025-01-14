#pragma once

#include <glm/glm.hpp>

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

struct Vector3_Aligned16B
{
   alignas(16) Vector3 value{};
};

using Matrix4x4 = glm::mat4x4;

[[nodiscard]] constexpr u32 divide_rounded_up(const u32 nominator, const u32 denominator)
{
   if ((nominator % denominator) == 0)
      return nominator / denominator;
   return 1 + (nominator / denominator);
}

constexpr auto g_pi = 3.14159265358979323846f;

}// namespace triglav
