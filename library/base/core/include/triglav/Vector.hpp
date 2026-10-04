#pragma once

#include "Macros.hpp"
#include "MathConstants.hpp"

#define TG_VECTOR_COMPONENTS_4 x, y, z, w
#define TG_VECTOR_COMPONENTS_3 x, y, z
#define TG_VECTOR_COMPONENTS_2 x, y
#define TG_VECTOR_COMPONENTS(count) TG_CONCAT(TG_VECTOR_COMPONENTS_, count)

#define TG_VECTOR_SUFFIX_u8 b
#define TG_VECTOR_SUFFIX_u32 u
#define TG_VECTOR_SUFFIX_i32 i
#define TG_VECTOR_SUFFIX_float
#define TG_VECTOR_SUFFIX(vec_type) TG_CONCAT(TG_VECTOR_SUFFIX_, vec_type)

#define TG_DECLARE_VECTOR_CONSTRUCTORS_2(vec_suffix)                           \
   constexpr TG_CONCAT(Vector2, vec_suffix)() = default;                       \
   constexpr TG_CONCAT(Vector2, vec_suffix)(ComponentType x, ComponentType y); \
   constexpr TG_CONCAT(Vector2, vec_suffix)(ComponentType value);

#define TG_DECLARE_VECTOR_CONSTRUCTORS_3(vec_suffix)                                                     \
   constexpr TG_CONCAT(Vector3, vec_suffix)() = default;                                                 \
   constexpr TG_CONCAT(Vector3, vec_suffix)(ComponentType x, ComponentType y, ComponentType z);          \
   constexpr TG_CONCAT(Vector3, vec_suffix)(const TG_CONCAT(Vector2, vec_suffix) & xy, ComponentType z); \
   constexpr TG_CONCAT(Vector3, vec_suffix)(ComponentType x, const TG_CONCAT(Vector2, vec_suffix) & yz); \
   constexpr TG_CONCAT(Vector3, vec_suffix)(ComponentType value);

#define TG_DECLARE_VECTOR_CONSTRUCTORS_4(vec_suffix)                                                                               \
   constexpr TG_CONCAT(Vector4, vec_suffix)() = default;                                                                           \
   constexpr TG_CONCAT(Vector4, vec_suffix)(ComponentType x, ComponentType y, ComponentType z, ComponentType w);                   \
   constexpr TG_CONCAT(Vector4, vec_suffix)(const TG_CONCAT(Vector2, vec_suffix) & xy, ComponentType z, ComponentType w);          \
   constexpr TG_CONCAT(Vector4, vec_suffix)(ComponentType x, const TG_CONCAT(Vector2, vec_suffix) & yz, ComponentType w);          \
   constexpr TG_CONCAT(Vector4, vec_suffix)(ComponentType x, ComponentType y, const TG_CONCAT(Vector2, vec_suffix) & zw);          \
   constexpr TG_CONCAT(Vector4, vec_suffix)(const TG_CONCAT(Vector2, vec_suffix) & xy, const TG_CONCAT(Vector2, vec_suffix) & zw); \
   constexpr TG_CONCAT(Vector4, vec_suffix)(const TG_CONCAT(Vector3, vec_suffix) & xyz, ComponentType w);                          \
   constexpr TG_CONCAT(Vector4, vec_suffix)(ComponentType x, const TG_CONCAT(Vector3, vec_suffix) & yzw);                          \
   constexpr TG_CONCAT(Vector4, vec_suffix)(ComponentType value);

#define TG_DECLARE_VECTOR_CONSTRUCTORS(vec_comp_count, vec_suffix) TG_CONCAT(TG_DECLARE_VECTOR_CONSTRUCTORS_, vec_comp_count)(vec_suffix)

#define TG_DECLARE_VECTOR_COMPONENT_BINARY_OVERLOAD(binary_op) \
   [[nodiscard]] constexpr Self operator binary_op(const ComponentType value) const;

#define TG_DECLARE_VECTOR_BASE(vec_comp_type, vec_comp_count)                                  \
   static constexpr u32 COMPONENT_COUNT = vec_comp_count;                                      \
                                                                                               \
   using Self = TG_CONCAT(Vector, TG_CONCAT(vec_comp_count, TG_VECTOR_SUFFIX(vec_comp_type))); \
   using ComponentType = vec_comp_type;                                                        \
                                                                                               \
   union                                                                                       \
   {                                                                                           \
      vec_comp_type components[vec_comp_count]{};                                              \
      struct                                                                                   \
      {                                                                                        \
         vec_comp_type TG_VECTOR_COMPONENTS(vec_comp_count);                                   \
      };                                                                                       \
   };                                                                                          \
   TG_DECLARE_VECTOR_CONSTRUCTORS(vec_comp_count, TG_VECTOR_SUFFIX(vec_comp_type))             \
                                                                                               \
   constexpr Self& operator+=(const Self& other);                                              \
   constexpr Self& operator-=(const Self& other);                                              \
   constexpr Self& operator*=(const Self& other);                                              \
   constexpr Self& operator/=(const Self& other);                                              \
                                                                                               \
   constexpr Self& operator+=(const ComponentType value);                                      \
   constexpr Self& operator-=(const ComponentType value);                                      \
   constexpr Self& operator*=(const ComponentType value);                                      \
   constexpr Self& operator/=(const ComponentType value);                                      \
                                                                                               \
   [[nodiscard]] constexpr Self operator+(const Self& other) const;                            \
   [[nodiscard]] constexpr Self operator-(const Self& other) const;                            \
   [[nodiscard]] constexpr Self operator*(const Self& other) const;                            \
   [[nodiscard]] constexpr Self operator/(const Self& other) const;                            \
                                                                                               \
   [[nodiscard]] constexpr Self operator+(const ComponentType value) const;                    \
   [[nodiscard]] constexpr Self operator-(const ComponentType value) const;                    \
   [[nodiscard]] constexpr Self operator*(const ComponentType value) const;                    \
   [[nodiscard]] constexpr Self operator/(const ComponentType value) const;                    \
                                                                                               \
   [[nodiscard]] constexpr Self operator-() const;                                             \
                                                                                               \
   [[nodiscard]] constexpr bool operator==(const Self& other) const;                           \
   [[nodiscard]] constexpr bool operator==(const ComponentType value) const;                   \
                                                                                               \
   [[nodiscard]] constexpr ComponentType operator[](u32 index) const;                          \
   [[nodiscard]] constexpr ComponentType& operator[](u32 index);                               \
   [[nodiscard]] constexpr ComponentType operator[](Axis axis) const;                          \
   [[nodiscard]] constexpr ComponentType& operator[](Axis Axis);                               \
                                                                                               \
   [[nodiscard]] constexpr ComponentType component_sum() const;                                \
   [[nodiscard]] constexpr Axis minor_axis() const;                                            \
   [[nodiscard]] constexpr Axis major_axis() const;                                            \
   [[nodiscard]] constexpr static Self from_axis(Axis axis);                                   \
                                                                                               \
   template<typename TDest>                                                                    \
   [[nodiscard]] constexpr TDest cast() const noexcept;


#define TG_DECLARE_VECTOR_FP(vec_comp_type, vec_comp_count)    \
   TG_DECLARE_VECTOR_BASE(vec_comp_type, vec_comp_count)       \
   [[nodiscard]] constexpr ComponentType length() const;       \
   [[nodiscard]] constexpr Self normalize() const;             \
   [[nodiscard]] constexpr float dot(const Self& other) const; \
   [[nodiscard]] constexpr Self degrees() const;               \
   [[nodiscard]] constexpr Self radians() const;               \
   [[nodiscard]] constexpr Self abs() const;

namespace triglav {

struct Vector2i;
struct Vector3i;
struct Vector4i;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
#elifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

struct Vector2b
{
   TG_DECLARE_VECTOR_BASE(u8, 2)
};

struct Vector3b
{
   TG_DECLARE_VECTOR_BASE(u8, 3)
};

struct Vector4b
{
   TG_DECLARE_VECTOR_BASE(u8, 4)
};

struct Vector2u
{
   TG_DECLARE_VECTOR_BASE(u32, 2)

   [[nodiscard]] constexpr operator Vector2i() const;
};

struct Vector3u
{
   TG_DECLARE_VECTOR_BASE(u32, 3)

   [[nodiscard]] constexpr operator Vector3i() const;
};
struct Vector4u
{
   TG_DECLARE_VECTOR_BASE(u32, 4)

   [[nodiscard]] constexpr operator Vector4i() const;
};
struct Vector2i
{
   TG_DECLARE_VECTOR_BASE(i32, 2)

   [[nodiscard]] constexpr operator Vector2u() const;
};
struct Vector3i
{
   TG_DECLARE_VECTOR_BASE(i32, 3)

   [[nodiscard]] constexpr operator Vector3u() const;
};
struct Vector4i
{
   TG_DECLARE_VECTOR_BASE(i32, 4)

   [[nodiscard]] constexpr operator Vector4u() const;
};

struct Vector2
{
   TG_DECLARE_VECTOR_FP(float, 2)
};

struct Vector3
{
   TG_DECLARE_VECTOR_FP(float, 3)

   [[nodiscard]] constexpr Vector3 cross(const Vector3& other) const;
   [[nodiscard]] constexpr Vector2 xy() const;
};

struct Vector4
{
   TG_DECLARE_VECTOR_FP(float, 4)

   [[nodiscard]] constexpr Vector3 xyz() const;
   [[nodiscard]] constexpr Vector2 xy() const;
};

#ifdef __clang__
#pragma clang diagnostic pop
#elifdef __GNUC__
#pragma GCC diagnostic pop
#endif

}// namespace triglav

#include "Vector.inl"