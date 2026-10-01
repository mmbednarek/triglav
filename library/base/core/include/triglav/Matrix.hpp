#pragma once

#include "Matrix.hpp"
#include "Vector.hpp"

namespace triglav {

struct Matrix4x4;
struct Matrix3x3;
struct Quaternion;

#define TG_MATRIX_MEMBERS_COLUMN_4(column) \
   ComponentType TG_CONCAT3(m, column, 0), TG_CONCAT3(m, column, 1), TG_CONCAT3(m, column, 2), TG_CONCAT3(m, column, 3);

#define TG_MATRIX_MEMBERS_COLUMN_3(column) ComponentType TG_CONCAT3(m, column, 0), TG_CONCAT3(m, column, 1), TG_CONCAT3(m, column, 2);

#define TG_MATRIX_MEMBERS_COLUMN(count, column) TG_CONCAT(TG_MATRIX_MEMBERS_COLUMN_, count)(column)

#define TG_MATRIX_MEMBERS_4(row_count)    \
   TG_MATRIX_MEMBERS_COLUMN(row_count, 0) \
   TG_MATRIX_MEMBERS_COLUMN(row_count, 1) \
   TG_MATRIX_MEMBERS_COLUMN(row_count, 2) \
   TG_MATRIX_MEMBERS_COLUMN(row_count, 3)

#define TG_MATRIX_MEMBERS_3(row_count)    \
   TG_MATRIX_MEMBERS_COLUMN(row_count, 0) \
   TG_MATRIX_MEMBERS_COLUMN(row_count, 1) \
   TG_MATRIX_MEMBERS_COLUMN(row_count, 2)

#define TG_MATRIX_MEMBERS(col_count, row_count) TG_CONCAT(TG_MATRIX_MEMBERS_, col_count)(row_count)

#define TG_DECLARE_MATRIX_FUNCTIONS(col_count, row_count)                           \
   static constexpr u32 COLUMN_COUNT = col_count;                                   \
   static constexpr u32 ROW_COUNT = row_count;                                      \
   using Self = TG_CONCAT(Matrix, TG_CONCAT3(col_count, x, row_count));             \
   using ColumnType = TG_CONCAT(Vector, col_count);                                 \
   using RowType = TG_CONCAT(Vector, row_count);                                    \
   using ComponentType = float;                                                     \
   static_assert(ColumnType::COMPONENT_COUNT == ROW_COUNT);                         \
   static_assert(RowType::COMPONENT_COUNT == COLUMN_COUNT);                         \
   union                                                                            \
   {                                                                                \
      ComponentType components[COLUMN_COUNT * ROW_COUNT]{};                         \
      ColumnType columns[COLUMN_COUNT];                                             \
      struct                                                                        \
      {                                                                             \
         TG_MATRIX_MEMBERS(col_count, row_count)                                    \
      };                                                                            \
   };                                                                               \
   [[nodiscard]] constexpr ComponentType value(const u32 col, const u32 row) const; \
   [[nodiscard]] constexpr ComponentType& value(const u32 col, const u32 row);      \
   [[nodiscard]] constexpr Self transpose() const;                                  \
   [[nodiscard]] constexpr ColumnType operator*(const ColumnType& vector) const;    \
   [[nodiscard]] constexpr ColumnType operator[](const u32 index) const;            \
   [[nodiscard]] constexpr ColumnType& operator[](const u32 index);                 \
   [[nodiscard]] constexpr RowType row(const u32 index) const;                      \
   [[nodiscard]] constexpr Self operator*(const Self& other) const;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
#elifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

struct Matrix4x4
{
   TG_DECLARE_MATRIX_FUNCTIONS(4, 4)

   constexpr Matrix4x4() = default;

   constexpr Matrix4x4(ColumnType a, ColumnType b, ColumnType c, ColumnType d);

   constexpr explicit Matrix4x4(float value);

   [[nodiscard]] constexpr Matrix4x4 inverse() const;
   [[nodiscard]] constexpr Matrix3x3 shrink() const;

   [[nodiscard]] constexpr static Matrix4x4 identity();
   [[nodiscard]] constexpr static Matrix4x4 translation(const Vector3& vec);
   [[nodiscard]] constexpr static Matrix4x4 rotation(const Quaternion& quat);
   [[nodiscard]] constexpr static Matrix4x4 scale(const Vector3& vec);
   [[nodiscard]] constexpr static Matrix4x4 look_at(const Vector3& eye, const Vector3& center, const Vector3& up) noexcept;
   [[nodiscard]] constexpr static Matrix4x4 orthographic_projection(float left, float right, float bottom, float top, float z_near,
                                                                    float z_far) noexcept;
   [[nodiscard]] constexpr static Matrix4x4 perspective_projection(float fov, float aspect_ratio, float z_near, float z_far) noexcept;
};

struct Matrix3x3
{
   TG_DECLARE_MATRIX_FUNCTIONS(3, 3)

   constexpr Matrix3x3() = default;
   constexpr Matrix3x3(ColumnType a, ColumnType b, ColumnType c);

   [[nodiscard]] constexpr Matrix3x3 inverse() const;
   [[nodiscard]] constexpr Matrix4x4 extend() const;

   [[nodiscard]] constexpr static Matrix3x3 identity();
   [[nodiscard]] constexpr static Matrix3x3 rotation(const Quaternion& quat);
   [[nodiscard]] constexpr static Matrix3x3 scale(const Vector3& vec);
};

#ifdef __clang__
#pragma clang diagnostic pop
#elifdef __GNUC__
#pragma GCC diagnostic pop
#endif

}// namespace triglav

#include "Matrix.inl"