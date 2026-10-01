#pragma once

namespace triglav {

constexpr auto PI = 3.14159265358979323846f;
constexpr auto g_pi = PI;
constexpr auto EPSILON = 1e-8f;

constexpr float radians_to_degrees(const float radians)
{
   return radians / PI * 180.0f;
}

constexpr float degrees_to_radians(const float degrees)
{
   return degrees / 180.0f * PI;
}

}// namespace triglav