#pragma once

namespace triglav {

struct Quaternion
{
   float x{}, y{}, z{}, w{};

   constexpr Quaternion() = default;
   constexpr Quaternion(float w, float x, float y, float z);

   [[nodiscard]] constexpr bool operator==(const Quaternion& rhs) const noexcept;
   constexpr Quaternion& operator*=(const Quaternion& rhs) noexcept;
   constexpr Quaternion& operator*=(float mult) noexcept;
   [[nodiscard]] constexpr Quaternion operator*(const Quaternion& rhs) const noexcept;
   [[nodiscard]] constexpr Quaternion operator/(float constant) const noexcept;
   [[nodiscard]] constexpr Quaternion operator-() const noexcept;
   [[nodiscard]] constexpr Vector3 operator*(const Vector3& rhs) const noexcept;

   [[nodiscard]] constexpr Vector3 euler_angles() const noexcept;
   [[nodiscard]] constexpr Quaternion normalize() const;
   [[nodiscard]] constexpr Quaternion inverse() const noexcept;

   [[nodiscard]] constexpr static Quaternion identity();
   [[nodiscard]] constexpr static Quaternion from_euler_angles(const Vector3& euler);
   [[nodiscard]] constexpr static Quaternion from_rotation_matrix(const Matrix3x3& rot_mat) noexcept;
   [[nodiscard]] constexpr static Quaternion angle_axis(float angle, const Vector3& v);
   [[nodiscard]] constexpr static Quaternion angle_axis(float angle, Axis axis);
   [[nodiscard]] constexpr static Quaternion from_oriented_vector(const Vector3& source, const Vector3& target) noexcept;
};

}// namespace triglav

#include "Quaternion.inl"