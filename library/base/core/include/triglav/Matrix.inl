#pragma once

#include "Quaternion.hpp"

namespace triglav {

#define TG_IMPLEMENT_MATRIX_FUNCTIONS(matrix_type)                                                           \
   [[nodiscard]] constexpr matrix_type::ComponentType matrix_type::value(const u32 col, const u32 row) const \
   {                                                                                                         \
      return this->components[col * ROW_COUNT + row];                                                        \
   }                                                                                                         \
   [[nodiscard]] constexpr matrix_type::ComponentType& matrix_type::value(const u32 col, const u32 row)      \
   {                                                                                                         \
      return this->components[col * ROW_COUNT + row];                                                        \
   }                                                                                                         \
   [[nodiscard]] constexpr matrix_type matrix_type::transpose() const                                        \
   {                                                                                                         \
      Self result{};                                                                                         \
      for (u32 c = 0; c < COLUMN_COUNT; ++c) {                                                               \
         for (u32 r = 0; r < ROW_COUNT; ++r) {                                                               \
            result.value(r, c) = this->value(c, r);                                                          \
         }                                                                                                   \
      }                                                                                                      \
      return result;                                                                                         \
   }                                                                                                         \
   constexpr matrix_type::ColumnType matrix_type::operator*(const ColumnType& vector) const                  \
   {                                                                                                         \
      ColumnType result{};                                                                                   \
      for (u32 col = 0; col < COLUMN_COUNT; ++col) {                                                         \
         result += this->columns[col] * vector.components[col];                                              \
      }                                                                                                      \
      return result;                                                                                         \
   }                                                                                                         \
   [[nodiscard]] constexpr matrix_type::ColumnType matrix_type::operator[](const u32 index) const            \
   {                                                                                                         \
      return this->columns[index];                                                                           \
   }                                                                                                         \
   [[nodiscard]] constexpr matrix_type::ColumnType& matrix_type::operator[](const u32 index)                 \
   {                                                                                                         \
      return this->columns[index];                                                                           \
   }                                                                                                         \
   [[nodiscard]] constexpr matrix_type::RowType matrix_type::row(const u32 index) const                      \
   {                                                                                                         \
      RowType result{};                                                                                      \
      for (u32 column = 0; column < COLUMN_COUNT; column++) {                                                \
         result.components[column] = this->value(column, index);                                             \
      }                                                                                                      \
      return result;                                                                                         \
   }                                                                                                         \
   [[nodiscard]] constexpr matrix_type::Self matrix_type::operator*(const Self& other) const                 \
   {                                                                                                         \
      Self result{};                                                                                         \
      for (u32 col = 0; col < COLUMN_COUNT; ++col) {                                                         \
         for (u32 row = 0; row < ROW_COUNT; ++row) {                                                         \
            result[col][row] = (this->row(row) * other[col]).component_sum();                                \
         }                                                                                                   \
      }                                                                                                      \
      return result;                                                                                         \
   }

TG_IMPLEMENT_MATRIX_FUNCTIONS(Matrix4x4)

constexpr Matrix4x4::Matrix4x4(ColumnType a, ColumnType b, ColumnType c, ColumnType d) :
    columns{a, b, c, d}
{
}

constexpr Matrix4x4::Matrix4x4(float value) :
    components{
       // clang-format off
       value, 0, 0, 0,
       0, value, 0, 0,
       0, 0, value, 0,
       0, 0, 0, value,
       // clang-format off
    }
{
}

constexpr Matrix4x4 Matrix4x4::inverse() const
{
   // Compute sub-determinants (2x2 minors) to minimize redundant operations
   const float A2323 = m22 * m33 - m23 * m32;
   const float A1323 = m21 * m33 - m23 * m31;
   const float A1223 = m21 * m32 - m22 * m31;
   const float A0323 = m20 * m33 - m23 * m30;
   const float A0223 = m20 * m32 - m22 * m30;
   const float A0123 = m20 * m31 - m21 * m30;
   const float A2313 = m12 * m33 - m13 * m32;
   const float A1313 = m11 * m33 - m13 * m31;
   const float A1213 = m11 * m32 - m12 * m31;
   const float A2312 = m12 * m23 - m13 * m22;
   const float A1312 = m11 * m23 - m13 * m21;
   const float A1212 = m11 * m22 - m12 * m21;
   const float A0313 = m10 * m33 - m13 * m30;
   const float A0213 = m10 * m32 - m12 * m30;
   const float A0312 = m10 * m23 - m13 * m20;
   const float A0212 = m10 * m22 - m12 * m20;
   const float A0113 = m10 * m31 - m11 * m30;
   const float A0112 = m10 * m21 - m11 * m20;

   // Compute overall determinant using Laplace expansion along first row/column
   const float det = m00 * (m11 * A2323 - m12 * A1323 + m13 * A1223) - m01 * (m10 * A2323 - m12 * A0323 + m13 * A0223) +
                     m02 * (m10 * A1323 - m11 * A0323 + m13 * A0123) - m03 * (m10 * A1223 - m11 * A0223 + m12 * A0123);

   // If determinant is near zero, the matrix is non-invertible
   if (det == 0.0f) {
      return Matrix4x4{};// Return identity or uninitialized matrix depending on your design
   }

   const float inv_det = 1.0f / det;

   Matrix4x4 result{};

   // Calculate adjugate matrix transposed (column-major assignment)
   result.m00 = (m11 * A2323 - m12 * A1323 + m13 * A1223) * inv_det;
   result.m01 = -(m01 * A2323 - m02 * A1323 + m03 * A1223) * inv_det;
   result.m02 = (m01 * A2313 - m02 * A1313 + m03 * A1213) * inv_det;
   result.m03 = -(m01 * A2312 - m02 * A1312 + m03 * A1212) * inv_det;

   result.m10 = -(m10 * A2323 - m12 * A0323 + m13 * A0223) * inv_det;
   result.m11 = (m00 * A2323 - m02 * A0323 + m03 * A0223) * inv_det;
   result.m12 = -(m00 * A2313 - m02 * A0313 + m03 * A0213) * inv_det;
   result.m13 = (m00 * A2312 - m02 * A0312 + m03 * A0212) * inv_det;

   result.m20 = (m10 * A1323 - m11 * A0323 + m13 * A0123) * inv_det;
   result.m21 = -(m00 * A1323 - m01 * A0323 + m03 * A0123) * inv_det;
   result.m22 = (m00 * A1313 - m01 * A0313 + m03 * A0113) * inv_det;
   result.m23 = -(m00 * A1312 - m01 * A0312 + m03 * A0112) * inv_det;

   result.m30 = -(m10 * A1223 - m11 * A0223 + m12 * A0123) * inv_det;
   result.m31 = (m00 * A1223 - m01 * A0223 + m02 * A0123) * inv_det;
   result.m32 = -(m00 * A1213 - m01 * A0213 + m02 * A0113) * inv_det;
   result.m33 = (m00 * A1212 - m01 * A0212 + m02 * A0112) * inv_det;

   return result;
}

constexpr Matrix3x3 Matrix4x4::shrink() const
{
   // clang-format off
   return {
         {m00, m01, m02},
         {m10, m11, m12},
         {m20, m21, m22},
      };
   // clang-format on
}

constexpr Matrix4x4 Matrix4x4::identity()
{
   // clang-format off
      return Matrix4x4{
            {1, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 1},
         };
   // clang-format on
}

constexpr Matrix4x4 Matrix4x4::translation(const Vector3& vec)
{
   // clang-format off
      return Matrix4x4{
         {1, 0, 0, 0,},
         {0, 1, 0, 0,},
         {0, 0, 1, 0,},
         {vec.x, vec.y, vec.z, 1,},
      };
   // clang-format on
}

constexpr Matrix4x4 Matrix4x4::rotation(const Quaternion& quat)
{
   return Matrix3x3::rotation(quat).extend();
}

constexpr Matrix4x4 Matrix4x4::scale(const Vector3& vec)
{
   return Matrix3x3::scale(vec).extend();
}

constexpr Matrix4x4 Matrix4x4::look_at(const Vector3& eye, const Vector3& center, const Vector3& up) noexcept
{
   const Vector3 f((center - eye).normalize());
   const Vector3 s(f.cross(up).normalize());
   const Vector3 u(s.cross(f));

   Matrix4x4 result = identity();
   result[0][0] = s.x;
   result[1][0] = s.y;
   result[2][0] = s.z;
   result[0][1] = u.x;
   result[1][1] = u.y;
   result[2][1] = u.z;
   result[0][2] = -f.x;
   result[1][2] = -f.y;
   result[2][2] = -f.z;
   result[3][0] = -s.dot(eye);
   result[3][1] = -u.dot(eye);
   result[3][2] = f.dot(eye);
   return result;
}

constexpr Matrix4x4 Matrix4x4::orthographic_projection(const float left, const float right, const float bottom, const float top,
                                                       const float z_near, const float z_far) noexcept
{
   Matrix4x4 result = identity();
   result[0][0] = 2.0f / (right - left);
   result[1][1] = 2.0f / (top - bottom);
   result[2][2] = -2.0f / (z_far - z_near);
   result[3][0] = -(right + left) / (right - left);
   result[3][1] = -(top + bottom) / (top - bottom);
   result[3][2] = -(z_far + z_near) / (z_far - z_near);
   return result;
}

constexpr Matrix4x4 Matrix4x4::perspective_projection(const float fov, const float aspect_ratio, const float z_near,
                                                      const float z_far) noexcept
{
   if (std::abs(aspect_ratio - std::numeric_limits<float>::epsilon()) <= 0.0f) {
      return Matrix4x4(0);
   }

   const float tan_half_fov = std::tan(fov / 2.0f);

   Matrix4x4 result{0.0f};
   result[0][0] = 1.0f / (aspect_ratio * tan_half_fov);
   result[1][1] = 1.0f / (tan_half_fov);
   result[2][2] = -(z_far + z_near) / (z_far - z_near);
   result[2][3] = -1.0f;
   result[3][2] = -(2.0f * z_far * z_near) / (z_far - z_near);
   return result;
}

TG_IMPLEMENT_MATRIX_FUNCTIONS(Matrix3x3)

constexpr Matrix3x3::Matrix3x3(ColumnType a, ColumnType b, ColumnType c) :
    columns{a, b, c}
{
}

constexpr Matrix3x3 Matrix3x3::inverse() const
{
   const float det_rcp = 1.0f / (+m00 * (m11 * m22 - m21 * m12) - m10 * (m01 * m22 - m21 * m02) + m20 * (m01 * m12 - m11 * m02));

   Matrix3x3 result;
   result.m00 = +(m11 * m22 - m21 * m12) * det_rcp;
   result.m10 = -(m10 * m22 - m20 * m12) * det_rcp;
   result.m20 = +(m10 * m21 - m20 * m11) * det_rcp;
   result.m01 = -(m01 * m22 - m21 * m02) * det_rcp;
   result.m11 = +(m00 * m22 - m20 * m02) * det_rcp;
   result.m21 = -(m00 * m21 - m20 * m01) * det_rcp;
   result.m02 = +(m01 * m12 - m11 * m02) * det_rcp;
   result.m12 = -(m00 * m12 - m10 * m02) * det_rcp;
   result.m22 = +(m00 * m11 - m10 * m01) * det_rcp;

   return result;
}

constexpr Matrix4x4 Matrix3x3::extend() const
{
   // clang-format off
      return {
         {m00, m01, m02, 0},
         {m10, m11, m12, 0},
         {m20, m21, m22, 0},
         {0, 0, 0, 1},
      };
   // clang-format on
}

constexpr Matrix3x3 Matrix3x3::identity()
{
   return Matrix3x3{
      {1, 0, 0},
      {0, 1, 0},
      {0, 0, 1},
   };
}

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

constexpr Matrix3x3 Matrix3x3::scale(const Vector3& vec)
{
   // clang-format off
      return Matrix3x3{
         {vec.x, 0, 0},
         {0, vec.y, 0},
         {0, 0, vec.z},
      };
   // clang-format on
}

}// namespace triglav