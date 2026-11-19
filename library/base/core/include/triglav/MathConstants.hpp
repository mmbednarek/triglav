#pragma once

#include <numeric>

namespace triglav {

struct PIType
{
   static constexpr float float_value = std::bit_cast<float, u32>(0x40490fdb);
   static constexpr float float_rcp = std::bit_cast<float, u32>(0x3ea2f983);
   static constexpr double double_value = std::bit_cast<double, u64>(0x400921fb54442d18);
   static constexpr double double_rcp = std::bit_cast<double, u64>(0x3fd45f306dc9c883);

   [[nodiscard]] constexpr operator float() const
   {
      return float_value;
   }

   [[nodiscard]] constexpr operator double() const
   {
      return double_value;
   }

   [[nodiscard]] constexpr float operator/(const float denom) const
   {
      return float_value / denom;
   }

   [[nodiscard]] constexpr double operator/(const double denom) const
   {
      return double_rcp / denom;
   }

   [[nodiscard]] constexpr float operator*(const float rhs) const
   {
      return float_value * rhs;
   }

   [[nodiscard]] constexpr double operator*(const double rhs) const
   {
      return double_value * rhs;
   }

   [[nodiscard]] constexpr float operator+(const float rhs) const
   {
      return float_value + rhs;
   }

   [[nodiscard]] constexpr double operator+(const double rhs) const
   {
      return double_value + rhs;
   }

   [[nodiscard]] constexpr float operator-(const float rhs) const
   {
      return float_value - rhs;
   }

   [[nodiscard]] constexpr double operator-(const double rhs) const
   {
      return double_value - rhs;
   }

   [[nodiscard]] constexpr double operator-() const
   {
      return -double_value;
   }
};

[[nodiscard]] constexpr float operator/(const float nom, const PIType /*denom*/)
{
   return nom * PIType::float_rcp;
}

[[nodiscard]] constexpr double operator/(const double nom, const PIType /*denom*/)
{
   return nom * PIType::double_rcp;
}

[[nodiscard]] constexpr float operator*(const float nom, const PIType /*denom*/)
{
   return nom * PIType::float_value;
}

[[nodiscard]] constexpr double operator*(const double nom, const PIType /*denom*/)
{
   return nom * PIType::double_value;
}

[[nodiscard]] constexpr float operator+(const float nom, const PIType /*denom*/)
{
   return nom + PIType::float_value;
}

[[nodiscard]] constexpr double operator+(const double nom, const PIType /*denom*/)
{
   return nom + PIType::double_value;
}

[[nodiscard]] constexpr float operator-(const float nom, const PIType /*denom*/)
{
   return nom - PIType::float_value;
}

[[nodiscard]] constexpr double operator-(const double nom, const PIType /*denom*/)
{
   return nom - PIType::double_value;
}

constexpr PIType PI;
constexpr auto g_pi = static_cast<float>(PI);
constexpr auto EPSILON = std::numeric_limits<float>::epsilon();
constexpr auto INF = std::numeric_limits<float>::infinity();

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