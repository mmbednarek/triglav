#pragma once

#include "Macros.hpp"

#define TG_VECTOR_COMPONENTS_4 x, y, z, w
#define TG_VECTOR_COMPONENTS_3 x, y, z
#define TG_VECTOR_COMPONENTS_2 x, y
#define TG_VECTOR_COMPONENTS(count) TG_CONCAT(TG_VECTOR_COMPONENTS_, count)

#define TG_IMPLEMENT_VECTOR_ASSIGNMENT_OVERLOAD(op)  \
   constexpr Self& operator op(const Self& other)    \
   {                                                 \
      for (u32 i = 0; i < COMPONENT_COUNT; ++i) {    \
         this->components[i] op other.components[i]; \
      }                                              \
      return *this;                                  \
   }

#define TG_IMPLEMENT_VECTOR_BINARY_OVERLOAD(assign_op, binary_op)           \
   [[nodiscard]] constexpr Self operator binary_op(const Self& other) const \
   {                                                                        \
      Self result = *this;                                                  \
      result assign_op other;                                               \
      return result;                                                        \
   }


#define TG_IMPLEMENT_VECTOR_COMPONENT_BINARY_OVERLOAD(assign_op, binary_op)         \
   [[nodiscard]] constexpr Self operator binary_op(const ComponentType value) const \
   {                                                                                \
      Self result = *this;                                                          \
      result assign_op value;                                                       \
      return result;                                                                \
   }

#define TG_IMPLEMENT_VECTOR_COMPONENT_ASSIGNMENT_OVERLOAD(op) \
   constexpr Self& operator op(const ComponentType value)     \
   {                                                          \
      for (ComponentType& dst : this->components) {           \
         dst op value;                                        \
      }                                                       \
      return *this;                                           \
   }

#define TG_IMPLEMENT_VECTOR_BASE(vec_name, vec_comp_type, vec_comp_count)               \
   static constexpr u32 COMPONENT_COUNT = vec_comp_count;                               \
   using Self = vec_name;                                                               \
   using ComponentType = vec_comp_type;                                                 \
   union                                                                                \
   {                                                                                    \
      vec_comp_type components[vec_comp_count]{};                                       \
      struct                                                                            \
      {                                                                                 \
         vec_comp_type TG_VECTOR_COMPONENTS(vec_comp_count);                            \
      };                                                                                \
   };                                                                                   \
   TG_IMPLEMENT_VECTOR_ASSIGNMENT_OVERLOAD(+=)                                          \
   TG_IMPLEMENT_VECTOR_ASSIGNMENT_OVERLOAD(-=)                                          \
   TG_IMPLEMENT_VECTOR_ASSIGNMENT_OVERLOAD(*=)                                          \
   TG_IMPLEMENT_VECTOR_ASSIGNMENT_OVERLOAD(/=)                                          \
   TG_IMPLEMENT_VECTOR_COMPONENT_ASSIGNMENT_OVERLOAD(+=)                                \
   TG_IMPLEMENT_VECTOR_COMPONENT_ASSIGNMENT_OVERLOAD(-=)                                \
   TG_IMPLEMENT_VECTOR_COMPONENT_ASSIGNMENT_OVERLOAD(*=)                                \
   TG_IMPLEMENT_VECTOR_COMPONENT_ASSIGNMENT_OVERLOAD(/=)                                \
   TG_IMPLEMENT_VECTOR_BINARY_OVERLOAD(+=, +)                                           \
   TG_IMPLEMENT_VECTOR_BINARY_OVERLOAD(-=, -)                                           \
   TG_IMPLEMENT_VECTOR_BINARY_OVERLOAD(*=, *)                                           \
   TG_IMPLEMENT_VECTOR_BINARY_OVERLOAD(/=, /)                                           \
   TG_IMPLEMENT_VECTOR_COMPONENT_BINARY_OVERLOAD(+=, +)                                 \
   TG_IMPLEMENT_VECTOR_COMPONENT_BINARY_OVERLOAD(-=, -)                                 \
   TG_IMPLEMENT_VECTOR_COMPONENT_BINARY_OVERLOAD(*=, *)                                 \
   TG_IMPLEMENT_VECTOR_COMPONENT_BINARY_OVERLOAD(/=, /)                                 \
   [[nodiscard]] constexpr ComponentType component_sum() const                          \
   {                                                                                    \
      ComponentType result{};                                                           \
      for (const ComponentType component : this->components) {                          \
         result += component;                                                           \
      }                                                                                 \
      return result;                                                                    \
   }                                                                                    \
   [[nodiscard]] constexpr bool operator==(const Self& other) const                     \
   {                                                                                    \
      for (u32 i = 0; i < COMPONENT_COUNT; ++i) {                                       \
         if (this->components[i] != other.components[i])                                \
            return false;                                                               \
      }                                                                                 \
      return true;                                                                      \
   }                                                                                    \
   [[nodiscard]] constexpr bool operator==(const ComponentType value) const             \
   {                                                                                    \
      for (const ComponentType comp : this->components) {                               \
         if (comp != value)                                                             \
            return false;                                                               \
      }                                                                                 \
      return true;                                                                      \
   }                                                                                    \
   [[nodiscard]] constexpr ComponentType operator[](const u32 index) const              \
   {                                                                                    \
      return this->components[index];                                                   \
   }                                                                                    \
   [[nodiscard]] constexpr ComponentType& operator[](const u32 index)                   \
   {                                                                                    \
      return this->components[index];                                                   \
   }                                                                                    \
   template<typename TDest>                                                             \
   [[nodiscard]] constexpr TDest cast() const noexcept                                  \
   {                                                                                    \
      TDest result{};                                                                   \
      for (u32 i = 0; i < std::min(TDest::COMPONENT_COUNT, COMPONENT_COUNT); ++i) {     \
         result.components[i] = static_cast<TDest::ComponentType>(this->components[i]); \
      }                                                                                 \
      return result;                                                                    \
   }


#define TG_IMPLEMENT_VECTOR_FP(vec_name, vec_comp_type, vec_comp_count)  \
   TG_IMPLEMENT_VECTOR_BASE(vec_name, vec_comp_type, vec_comp_count)     \
   [[nodiscard]] constexpr ComponentType length() const                  \
   {                                                                     \
      return std::sqrt(((*this) * (*this)).component_sum());             \
   }                                                                     \
   [[nodiscard]] constexpr Self normalize() const                        \
   {                                                                     \
      return (*this) / this->length();                                   \
   }                                                                     \
   [[nodiscard]] constexpr float dot(const Self& other) const            \
   {                                                                     \
      return ((*this) * other).component_sum();                          \
   }                                                                     \
   [[nodiscard]] constexpr Self degrees() const                          \
   {                                                                     \
      Self result{};                                                     \
      for (u32 i = 0; i < COMPONENT_COUNT; ++i) {                        \
         result.components[i] = radians_to_degrees(this->components[i]); \
      }                                                                  \
      return result;                                                     \
   }                                                                     \
   [[nodiscard]] constexpr Self radians() const                          \
   {                                                                     \
      Self result{};                                                     \
      for (u32 i = 0; i < COMPONENT_COUNT; ++i) {                        \
         result.components[i] = degrees_to_radians(this->components[i]); \
      }                                                                  \
      return result;                                                     \
   }                                                                     \
   [[nodiscard]] constexpr Self abs() const                              \
   {                                                                     \
      Self result{};                                                     \
      for (u32 i = 0; i < COMPONENT_COUNT; ++i) {                        \
         result.components[i] = std::abs(this->components[i]);           \
      }                                                                  \
      return result;                                                     \
   }


#define TG_IMPLEMENT_VECTOR_LHS_BINARY_OVERLOAD(vector_type, binary_op)                                                 \
   [[nodiscard]] constexpr vector_type operator binary_op(const vector_type::ComponentType lhs, const vector_type& rhs) \
   {                                                                                                                    \
      return rhs binary_op lhs;                                                                                         \
   }

#define TG_IMPLEMENT_VECTOR_NON_MEMBER_OVERLOADS(vector_type) \
   TG_IMPLEMENT_VECTOR_LHS_BINARY_OVERLOAD(vector_type, +)    \
   TG_IMPLEMENT_VECTOR_LHS_BINARY_OVERLOAD(vector_type, *)

#define TG_IMPLEMENT_VECTOR_CONVERSION(source_ty, dest_ty)                                \
   [[nodiscard]] constexpr source_ty::operator dest_ty() const                            \
   {                                                                                      \
      dest_ty result{};                                                                   \
      for (u32 i = 0; i < dest_ty::COMPONENT_COUNT; ++i) {                                \
         result.components[i] = static_cast<dest_ty::ComponentType>(this->components[i]); \
      }                                                                                   \
      return result;                                                                      \
   }

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
   TG_IMPLEMENT_VECTOR_BASE(Vector2b, u8, 2)
};

struct Vector3b
{
   TG_IMPLEMENT_VECTOR_BASE(Vector3b, u8, 3)
};

struct Vector4b
{
   TG_IMPLEMENT_VECTOR_BASE(Vector4b, u8, 4)
};

struct Vector2u
{
   TG_IMPLEMENT_VECTOR_BASE(Vector2u, u32, 2)

   [[nodiscard]] constexpr operator Vector2i() const;
};

struct Vector3u
{
   TG_IMPLEMENT_VECTOR_BASE(Vector3u, u32, 3)

   constexpr Vector3u() = default;

   constexpr Vector3u(const u32 x, const u32 y, const u32 z) :
       components{x, y, z}
   {
   }

   constexpr Vector3u(const Vector2u a, const u32 z) :
       components{a.x, a.y, z}
   {
   }

   constexpr Vector3u(const u32 x, const Vector2u a) :
       components{x, a.x, a.y}
   {
   }

   constexpr Vector3u(const u32 v) :
       components{v, v, v}
   {
   }

   [[nodiscard]] constexpr operator Vector3i() const;
};
struct Vector4u
{
   TG_IMPLEMENT_VECTOR_BASE(Vector4u, u32, 4)

   [[nodiscard]] constexpr operator Vector4i() const;
};
struct Vector2i
{
   TG_IMPLEMENT_VECTOR_BASE(Vector2i, i32, 2)

   constexpr Vector2i() = default;

   constexpr Vector2i(const i32 x, const i32 y) :
       x{x},
       y{y}
   {
   }

   constexpr Vector2i(const i32 v) :
       x{v},
       y{v}
   {
   }

   [[nodiscard]] constexpr operator Vector2u() const;
};
struct Vector3i
{
   TG_IMPLEMENT_VECTOR_BASE(Vector3i, i32, 3)

   constexpr Vector3i() = default;

   constexpr Vector3i(const i32 x, const i32 y, const i32 z) :
       components{x, y, z}
   {
   }

   constexpr Vector3i(const Vector2i a, const i32 z) :
       components{a.x, a.y, z}
   {
   }

   constexpr Vector3i(const i32 x, const Vector2i a) :
       components{x, a.x, a.y}
   {
   }

   constexpr Vector3i(const i32 v) :
       components{v, v, v}
   {
   }

   [[nodiscard]] constexpr operator Vector3u() const;
};
struct Vector4i
{
   TG_IMPLEMENT_VECTOR_BASE(Vector4i, i32, 4)

   [[nodiscard]] constexpr operator Vector4u() const;
};

struct Vector2
{
   TG_IMPLEMENT_VECTOR_FP(Vector2, float, 2)

   constexpr Vector2() = default;

   constexpr Vector2(const float x, const float y) :
       components{x, y}
   {
   }

   constexpr Vector2(const float x) :
       components{x, x}
   {
   }
};

struct Vector3
{
   TG_IMPLEMENT_VECTOR_FP(Vector3, float, 3)

   constexpr Vector3() = default;

   constexpr Vector3(const float x, const float y, const float z) :
       components{x, y, z}
   {
   }

   constexpr Vector3(const Vector2 a, const float z) :
       components{a.x, a.y, z}
   {
   }

   constexpr Vector3(const float x, const Vector2 a) :
       components{x, a.x, a.y}
   {
   }

   constexpr Vector3(const float v) :
       components{v, v, v}
   {
   }

   [[nodiscard]] Vector3 cross(const Vector3& other) const
   {
      return Vector3{
         this->y * other.z - this->z * other.y,
         this->z * other.x - this->x * other.z,
         this->x * other.y - this->y * other.x,
      };
   }

   [[nodiscard]] Vector2 xy() const
   {
      return {this->x, this->y};
   }
};

struct Vector4
{
   TG_IMPLEMENT_VECTOR_FP(Vector4, float, 4)

   constexpr Vector4() = default;

   constexpr Vector4(const float x, const float y, const float z, const float w) :
       components{x, y, z, w}
   {
   }

   constexpr Vector4(const Vector2& a, const float z, const float w) :
       components{a.x, a.y, z, w}
   {
   }

   constexpr Vector4(const float x, const float y, const Vector2& v) :
       components{x, y, v.x, v.y}
   {
   }

   constexpr Vector4(const Vector2 a, const Vector2 b) :
       components{a.x, a.y, b.x, b.y}
   {
   }

   constexpr Vector4(const Vector3 a, const float w) :
       components{a.x, a.y, a.z, w}
   {
   }

   constexpr Vector4(const float v) :
       components{v, v, v, v}
   {
   }

   [[nodiscard]] Vector3 xyz() const
   {
      return {this->x, this->y, this->z};
   }

   [[nodiscard]] Vector2 xy() const
   {
      return {this->x, this->y};
   }
};

TG_IMPLEMENT_VECTOR_CONVERSION(Vector2i, Vector2u)
TG_IMPLEMENT_VECTOR_CONVERSION(Vector3i, Vector3u)
TG_IMPLEMENT_VECTOR_CONVERSION(Vector4i, Vector4u)
TG_IMPLEMENT_VECTOR_CONVERSION(Vector2u, Vector2i)
TG_IMPLEMENT_VECTOR_CONVERSION(Vector3u, Vector3i)
TG_IMPLEMENT_VECTOR_CONVERSION(Vector4u, Vector4i)

TG_IMPLEMENT_VECTOR_NON_MEMBER_OVERLOADS(Vector2b)
TG_IMPLEMENT_VECTOR_NON_MEMBER_OVERLOADS(Vector3b)
TG_IMPLEMENT_VECTOR_NON_MEMBER_OVERLOADS(Vector4b)
TG_IMPLEMENT_VECTOR_NON_MEMBER_OVERLOADS(Vector2u)
TG_IMPLEMENT_VECTOR_NON_MEMBER_OVERLOADS(Vector3u)
TG_IMPLEMENT_VECTOR_NON_MEMBER_OVERLOADS(Vector4u)
TG_IMPLEMENT_VECTOR_NON_MEMBER_OVERLOADS(Vector2i)
TG_IMPLEMENT_VECTOR_NON_MEMBER_OVERLOADS(Vector3i)
TG_IMPLEMENT_VECTOR_NON_MEMBER_OVERLOADS(Vector4i)
TG_IMPLEMENT_VECTOR_NON_MEMBER_OVERLOADS(Vector2)
TG_IMPLEMENT_VECTOR_NON_MEMBER_OVERLOADS(Vector3)
TG_IMPLEMENT_VECTOR_NON_MEMBER_OVERLOADS(Vector4)

#ifdef __clang__
#pragma clang diagnostic pop
#elifdef __GNUC__
#pragma GCC diagnostic pop
#endif

}// namespace triglav