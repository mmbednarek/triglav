#pragma once

namespace triglav {

struct Vector2b;
struct Vector3b;
struct Vector4b;
struct Vector2u;
struct Vector3u;
struct Vector4u;
struct Vector2i;
struct Vector3i;
struct Vector4i;
struct Vector2;
struct Vector3;
struct Vector4;

#define TG_IMPLEMENT_VECTOR_ASSIGNMENT_OVERLOAD(vec_name, op)   \
   constexpr vec_name& vec_name::operator op(const Self& other) \
   {                                                            \
      for (u32 i = 0; i < COMPONENT_COUNT; ++i) {               \
         this->components[i] op other.components[i];            \
      }                                                         \
      return *this;                                             \
   }

#define TG_IMPLEMENT_VECTOR_COMPONENT_ASSIGNMENT_OVERLOAD(vec_name, op) \
   constexpr vec_name& vec_name::operator op(const ComponentType value) \
   {                                                                    \
      for (ComponentType& dst : this->components) {                     \
         dst op value;                                                  \
      }                                                                 \
      return *this;                                                     \
   }

#define TG_IMPLEMENT_VECTOR_BINARY_OVERLOAD(vec_name, assign_op, binary_op) \
   constexpr vec_name vec_name::operator binary_op(const Self& other) const \
   {                                                                        \
      Self result = *this;                                                  \
      result assign_op other;                                               \
      return result;                                                        \
   }


#define TG_IMPLEMENT_VECTOR_COMPONENT_BINARY_OVERLOAD(vec_name, assign_op, binary_op) \
   constexpr vec_name vec_name::operator binary_op(const ComponentType value) const   \
   {                                                                                  \
      Self result = *this;                                                            \
      result assign_op value;                                                         \
      return result;                                                                  \
   }

#define TG_IMPLEMENT_VECTOR_CONSTRUCTORS_2(vec_suffix)                                                                        \
   constexpr TG_CONCAT(Vector2, vec_suffix)::TG_CONCAT(Vector2, vec_suffix)(const ComponentType x_, const ComponentType y_) : \
       x{x_},                                                                                                                 \
       y{y_}                                                                                                                  \
   {                                                                                                                          \
   }                                                                                                                          \
   constexpr TG_CONCAT(Vector2, vec_suffix)::TG_CONCAT(Vector2, vec_suffix)(const ComponentType value) :                      \
       x{value},                                                                                                              \
       y{value}                                                                                                               \
   {                                                                                                                          \
   }

#define TG_IMPLEMENT_VECTOR_CONSTRUCTORS_3(vec_suffix)                                                                      \
   constexpr TG_CONCAT(Vector3, vec_suffix)::TG_CONCAT(Vector3, vec_suffix)(const ComponentType x_, const ComponentType y_, \
                                                                            const ComponentType z_) :                       \
       x{x_},                                                                                                               \
       y{y_},                                                                                                               \
       z{z_}                                                                                                                \
   {                                                                                                                        \
   }                                                                                                                        \
   constexpr TG_CONCAT(Vector3, vec_suffix)::TG_CONCAT(Vector3, vec_suffix)(const TG_CONCAT(Vector2, vec_suffix) & xy_,     \
                                                                            const ComponentType z_) :                       \
       x{xy_.x},                                                                                                            \
       y{xy_.y},                                                                                                            \
       z{z_}                                                                                                                \
   {                                                                                                                        \
   }                                                                                                                        \
   constexpr TG_CONCAT(Vector3, vec_suffix)::TG_CONCAT(Vector3, vec_suffix)(const ComponentType x_,                         \
                                                                            const TG_CONCAT(Vector2, vec_suffix) & yz_) :   \
       x{x_},                                                                                                               \
       y{yz_.x},                                                                                                            \
       z{yz_.y}                                                                                                             \
   {                                                                                                                        \
   }                                                                                                                        \
   constexpr TG_CONCAT(Vector3, vec_suffix)::TG_CONCAT(Vector3, vec_suffix)(const ComponentType value) :                    \
       x{value},                                                                                                            \
       y{value},                                                                                                            \
       z{value}                                                                                                             \
   {                                                                                                                        \
   }

#define TG_IMPLEMENT_VECTOR_CONSTRUCTORS_4(vec_suffix)                                                                        \
   constexpr TG_CONCAT(Vector4, vec_suffix)::TG_CONCAT(Vector4, vec_suffix)(const ComponentType x_, const ComponentType y_,   \
                                                                            const ComponentType z_, const ComponentType w_) : \
       x{x_},                                                                                                                 \
       y{y_},                                                                                                                 \
       z{z_},                                                                                                                 \
       w{w_}                                                                                                                  \
   {                                                                                                                          \
   }                                                                                                                          \
   constexpr TG_CONCAT(Vector4, vec_suffix)::TG_CONCAT(Vector4, vec_suffix)(const TG_CONCAT(Vector2, vec_suffix) & xy_,       \
                                                                            const ComponentType z_, const ComponentType w_) : \
       x{xy_.x},                                                                                                              \
       y{xy_.y},                                                                                                              \
       z{z_},                                                                                                                 \
       w{w_}                                                                                                                  \
   {                                                                                                                          \
   }                                                                                                                          \
   constexpr TG_CONCAT(Vector4, vec_suffix)::TG_CONCAT(Vector4, vec_suffix)(                                                  \
      const ComponentType x_, const TG_CONCAT(Vector2, vec_suffix) & yz_, const ComponentType w_) :                           \
       x{x_},                                                                                                                 \
       y{yz_.x},                                                                                                              \
       z{yz_.y},                                                                                                              \
       w{w_}                                                                                                                  \
   {                                                                                                                          \
   }                                                                                                                          \
   constexpr TG_CONCAT(Vector4, vec_suffix)::TG_CONCAT(Vector4, vec_suffix)(const ComponentType x_, const ComponentType y_,   \
                                                                            const TG_CONCAT(Vector2, vec_suffix) & zw_) :     \
       x{x_},                                                                                                                 \
       y{y_},                                                                                                                 \
       z{zw_.x},                                                                                                              \
       w{zw_.y}                                                                                                               \
   {                                                                                                                          \
   }                                                                                                                          \
   constexpr TG_CONCAT(Vector4, vec_suffix)::TG_CONCAT(Vector4, vec_suffix)(const TG_CONCAT(Vector2, vec_suffix) & xy_,       \
                                                                            const TG_CONCAT(Vector2, vec_suffix) & zw_) :     \
       x{xy_.x},                                                                                                              \
       y{xy_.y},                                                                                                              \
       z{zw_.x},                                                                                                              \
       w{zw_.y}                                                                                                               \
   {                                                                                                                          \
   }                                                                                                                          \
   constexpr TG_CONCAT(Vector4, vec_suffix)::TG_CONCAT(Vector4, vec_suffix)(const TG_CONCAT(Vector3, vec_suffix) & xyz_,      \
                                                                            const ComponentType w_) :                         \
       x{xyz_.x},                                                                                                             \
       y{xyz_.y},                                                                                                             \
       z{xyz_.z},                                                                                                             \
       w{w_}                                                                                                                  \
   {                                                                                                                          \
   }                                                                                                                          \
   constexpr TG_CONCAT(Vector4, vec_suffix)::TG_CONCAT(Vector4, vec_suffix)(const ComponentType x_,                           \
                                                                            const TG_CONCAT(Vector3, vec_suffix) & yzw_) :    \
       x{x_},                                                                                                                 \
       y{yzw_.x},                                                                                                             \
       z{yzw_.y},                                                                                                             \
       w{yzw_.z}                                                                                                              \
   {                                                                                                                          \
   }                                                                                                                          \
   constexpr TG_CONCAT(Vector4, vec_suffix)::TG_CONCAT(Vector4, vec_suffix)(const ComponentType value) :                      \
       x{value},                                                                                                              \
       y{value},                                                                                                              \
       z{value},                                                                                                              \
       w{value}                                                                                                               \
   {                                                                                                                          \
   }

#define TG_IMPLEMENT_VECTOR_CONSTRUCTORS(vec_comp_count, vec_suffix) \
   TG_CONCAT(TG_IMPLEMENT_VECTOR_CONSTRUCTORS_, vec_comp_count)(vec_suffix)

#define TG_IMPLEMENT_VECTOR_FUNCTIONS(vec_name, vec_comp_count, vec_suffix)             \
   TG_IMPLEMENT_VECTOR_CONSTRUCTORS(vec_comp_count, vec_suffix)                         \
   TG_IMPLEMENT_VECTOR_ASSIGNMENT_OVERLOAD(vec_name, +=)                                \
   TG_IMPLEMENT_VECTOR_ASSIGNMENT_OVERLOAD(vec_name, -=)                                \
   TG_IMPLEMENT_VECTOR_ASSIGNMENT_OVERLOAD(vec_name, *=)                                \
   TG_IMPLEMENT_VECTOR_ASSIGNMENT_OVERLOAD(vec_name, /=)                                \
   TG_IMPLEMENT_VECTOR_COMPONENT_ASSIGNMENT_OVERLOAD(vec_name, +=)                      \
   TG_IMPLEMENT_VECTOR_COMPONENT_ASSIGNMENT_OVERLOAD(vec_name, -=)                      \
   TG_IMPLEMENT_VECTOR_COMPONENT_ASSIGNMENT_OVERLOAD(vec_name, *=)                      \
   TG_IMPLEMENT_VECTOR_COMPONENT_ASSIGNMENT_OVERLOAD(vec_name, /=)                      \
   TG_IMPLEMENT_VECTOR_BINARY_OVERLOAD(vec_name, +=, +)                                 \
   TG_IMPLEMENT_VECTOR_BINARY_OVERLOAD(vec_name, -=, -)                                 \
   TG_IMPLEMENT_VECTOR_BINARY_OVERLOAD(vec_name, *=, *)                                 \
   TG_IMPLEMENT_VECTOR_BINARY_OVERLOAD(vec_name, /=, /)                                 \
   TG_IMPLEMENT_VECTOR_COMPONENT_BINARY_OVERLOAD(vec_name, +=, +)                       \
   TG_IMPLEMENT_VECTOR_COMPONENT_BINARY_OVERLOAD(vec_name, -=, -)                       \
   TG_IMPLEMENT_VECTOR_COMPONENT_BINARY_OVERLOAD(vec_name, *=, *)                       \
   TG_IMPLEMENT_VECTOR_COMPONENT_BINARY_OVERLOAD(vec_name, /=, /)                       \
   TG_IMPLEMENT_VECTOR_NON_MEMBER_OVERLOADS(vec_name)                                   \
                                                                                        \
   constexpr vec_name vec_name::operator-() const                                       \
   {                                                                                    \
      Self result = *this;                                                              \
      for (ComponentType& comp : result.components) {                                   \
         comp = -comp;                                                                  \
      }                                                                                 \
      return result;                                                                    \
   }                                                                                    \
   constexpr bool vec_name::operator==(const Self& other) const                         \
   {                                                                                    \
      for (u32 i = 0; i < COMPONENT_COUNT; ++i) {                                       \
         if (this->components[i] != other.components[i])                                \
            return false;                                                               \
      }                                                                                 \
      return true;                                                                      \
   }                                                                                    \
   constexpr bool vec_name::operator==(const ComponentType value) const                 \
   {                                                                                    \
      for (const ComponentType comp : this->components) {                               \
         if (comp != value)                                                             \
            return false;                                                               \
      }                                                                                 \
      return true;                                                                      \
   }                                                                                    \
   constexpr vec_name::ComponentType vec_name::operator[](const u32 index) const        \
   {                                                                                    \
      return this->components[index];                                                   \
   }                                                                                    \
   constexpr vec_name::ComponentType& vec_name::operator[](const u32 index)             \
   {                                                                                    \
      return this->components[index];                                                   \
   }                                                                                    \
   constexpr vec_name::ComponentType vec_name::operator[](const Axis axis) const        \
   {                                                                                    \
      return this->components[static_cast<u32>(axis)];                                  \
   }                                                                                    \
   constexpr vec_name::ComponentType& vec_name::operator[](const Axis axis)             \
   {                                                                                    \
      return this->components[static_cast<u32>(axis)];                                  \
   }                                                                                    \
   constexpr vec_name::ComponentType vec_name::component_sum() const                    \
   {                                                                                    \
      ComponentType result{};                                                           \
      for (const ComponentType component : this->components) {                          \
         result += component;                                                           \
      }                                                                                 \
      return result;                                                                    \
   }                                                                                    \
   constexpr Axis vec_name::minor_axis() const                                          \
   {                                                                                    \
      u32 result = 0;                                                                   \
      for (u32 i = 1; i < COMPONENT_COUNT; ++i) {                                       \
         if (this->components[i] < this->components[result]) {                          \
            result = i;                                                                 \
         }                                                                              \
      }                                                                                 \
      return static_cast<Axis>(result);                                                 \
   }                                                                                    \
   constexpr Axis vec_name::major_axis() const                                          \
   {                                                                                    \
      u32 result = 0;                                                                   \
      for (u32 i = 1; i < COMPONENT_COUNT; ++i) {                                       \
         if (this->components[i] > this->components[result]) {                          \
            result = i;                                                                 \
         }                                                                              \
      }                                                                                 \
      return static_cast<Axis>(result);                                                 \
   }                                                                                    \
   constexpr vec_name vec_name::from_axis(const Axis axis)                              \
   {                                                                                    \
      vec_name result{};                                                                \
      result[axis] = static_cast<ComponentType>(1);                                     \
      return result;                                                                    \
   }                                                                                    \
   template<typename TDest>                                                             \
   constexpr TDest vec_name::cast() const noexcept                                      \
   {                                                                                    \
      TDest result{};                                                                   \
      for (u32 i = 0; i < std::min(TDest::COMPONENT_COUNT, COMPONENT_COUNT); ++i) {     \
         result.components[i] = static_cast<TDest::ComponentType>(this->components[i]); \
      }                                                                                 \
      return result;                                                                    \
   }

#define TG_IMPLEMENT_VECTOR_FP_FUNCTIONS(vec_name, vec_comp_count, vec_suffix) \
   TG_IMPLEMENT_VECTOR_FUNCTIONS(vec_name, vec_comp_count, vec_suffix)         \
   constexpr vec_name::ComponentType vec_name::length() const                  \
   {                                                                           \
      return std::sqrt(((*this) * (*this)).component_sum());                   \
   }                                                                           \
   constexpr vec_name vec_name::normalize() const                              \
   {                                                                           \
      return (*this) / this->length();                                         \
   }                                                                           \
   constexpr float vec_name::dot(const Self& other) const                      \
   {                                                                           \
      return ((*this) * other).component_sum();                                \
   }                                                                           \
   constexpr vec_name vec_name::degrees() const                                \
   {                                                                           \
      Self result{};                                                           \
      for (u32 i = 0; i < COMPONENT_COUNT; ++i) {                              \
         result.components[i] = radians_to_degrees(this->components[i]);       \
      }                                                                        \
      return result;                                                           \
   }                                                                           \
   constexpr vec_name vec_name::radians() const                                \
   {                                                                           \
      Self result{};                                                           \
      for (u32 i = 0; i < COMPONENT_COUNT; ++i) {                              \
         result.components[i] = degrees_to_radians(this->components[i]);       \
      }                                                                        \
      return result;                                                           \
   }                                                                           \
   constexpr vec_name vec_name::abs() const                                    \
   {                                                                           \
      Self result{};                                                           \
      for (u32 i = 0; i < COMPONENT_COUNT; ++i) {                              \
         result.components[i] = std::abs(this->components[i]);                 \
      }                                                                        \
      return result;                                                           \
   }

#define TG_IMPLEMENT_VECTOR_LHS_BINARY_OVERLOAD(vector_type, binary_op)                                   \
   constexpr vector_type operator binary_op(const vector_type::ComponentType lhs, const vector_type& rhs) \
   {                                                                                                      \
      return rhs binary_op lhs;                                                                           \
   }

#define TG_IMPLEMENT_VECTOR_NON_MEMBER_OVERLOADS(vector_type) \
   TG_IMPLEMENT_VECTOR_LHS_BINARY_OVERLOAD(vector_type, +)    \
   TG_IMPLEMENT_VECTOR_LHS_BINARY_OVERLOAD(vector_type, *)

#define TG_IMPLEMENT_VECTOR_CONVERSION(source_ty, dest_ty)                                \
   constexpr source_ty::operator dest_ty() const                                          \
   {                                                                                      \
      dest_ty result{};                                                                   \
      for (u32 i = 0; i < dest_ty::COMPONENT_COUNT; ++i) {                                \
         result.components[i] = static_cast<dest_ty::ComponentType>(this->components[i]); \
      }                                                                                   \
      return result;                                                                      \
   }


TG_IMPLEMENT_VECTOR_CONVERSION(Vector2i, Vector2u)
TG_IMPLEMENT_VECTOR_CONVERSION(Vector3i, Vector3u)
TG_IMPLEMENT_VECTOR_CONVERSION(Vector4i, Vector4u)
TG_IMPLEMENT_VECTOR_CONVERSION(Vector2u, Vector2i)
TG_IMPLEMENT_VECTOR_CONVERSION(Vector3u, Vector3i)
TG_IMPLEMENT_VECTOR_CONVERSION(Vector4u, Vector4i)

TG_IMPLEMENT_VECTOR_FUNCTIONS(Vector2b, 2, b)
TG_IMPLEMENT_VECTOR_FUNCTIONS(Vector3b, 3, b)
TG_IMPLEMENT_VECTOR_FUNCTIONS(Vector4b, 4, b)
TG_IMPLEMENT_VECTOR_FUNCTIONS(Vector2u, 2, u)
TG_IMPLEMENT_VECTOR_FUNCTIONS(Vector3u, 3, u)
TG_IMPLEMENT_VECTOR_FUNCTIONS(Vector4u, 4, u)
TG_IMPLEMENT_VECTOR_FUNCTIONS(Vector2i, 2, i)
TG_IMPLEMENT_VECTOR_FUNCTIONS(Vector3i, 3, i)
TG_IMPLEMENT_VECTOR_FUNCTIONS(Vector4i, 4, i)
TG_IMPLEMENT_VECTOR_FP_FUNCTIONS(Vector2, 2, )
TG_IMPLEMENT_VECTOR_FP_FUNCTIONS(Vector3, 3, )
TG_IMPLEMENT_VECTOR_FP_FUNCTIONS(Vector4, 4, )

constexpr Vector3 Vector3::cross(const Vector3& other) const
{
   return Vector3{
      this->y * other.z - this->z * other.y,
      this->z * other.x - this->x * other.z,
      this->x * other.y - this->y * other.x,
   };
}

constexpr Vector2 Vector3::xy() const
{
   return {this->x, this->y};
}

constexpr Vector3 Vector4::xyz() const
{
   return {this->x, this->y, this->z};
}

constexpr Vector2 Vector4::xy() const
{
   return {this->x, this->y};
}

}// namespace triglav