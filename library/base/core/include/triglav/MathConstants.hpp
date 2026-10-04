#pragma once

#include <numeric>

namespace triglav {

constexpr auto PI = std::bit_cast<float, u32>(0x40490fdb);
constexpr auto g_pi = PI;
constexpr auto EPSILON = std::numeric_limits<float>::epsilon();

enum class Axis : u32
{
   X = 0,
   Y = 1,
   Z = 2,
   W = 3
};

constexpr float radians_to_degrees(const float radians)
{
   return radians / PI * 180.0f;
}

constexpr float degrees_to_radians(const float degrees)
{
   return degrees / 180.0f * PI;
}

}// namespace triglav