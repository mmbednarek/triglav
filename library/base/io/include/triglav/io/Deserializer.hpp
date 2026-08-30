#pragma once

#include "Stream.hpp"

#include "triglav/Math.hpp"

#include <string>
#include <type_traits>

#define TG_IO_SERIALIZATION_TYPES                            \
   TG_IO_TYPE(Vector2, read_vec2, write_vec2)                \
   TG_IO_TYPE(Vector3, read_vec3, write_vec3)                \
   TG_IO_TYPE(Vector4, read_vec4, write_vec4)                \
   TG_IO_TYPE(Matrix4x4, read_matrix4x4, write_matrix4x4)    \
   TG_IO_TYPE(mem_size, read_mem_size, write_mem_size)       \
   TG_IO_TYPE(Quaternion, read_quaternion, write_quaternion) \
   TG_IO_TYPE(Name, read_name, write_name)                   \
   TG_IO_TYPE(u64, read_u64, write_u64)                      \
   TG_IO_TYPE(u32, read_u32, write_u32)                      \
   TG_IO_TYPE(u16, read_u16, write_u16)                      \
   TG_IO_TYPE(u8, read_u8, write_u8)                         \
   TG_IO_TYPE(i64, read_i64, write_i64)                      \
   TG_IO_TYPE(i32, read_i32, write_i32)                      \
   TG_IO_TYPE(i16, read_i16, write_i16)                      \
   TG_IO_TYPE(i8, read_i8, write_i8)                         \
   TG_IO_TYPE(float, read_float, write_float)                \
   TG_IO_TYPE(double, read_double, write_double)

namespace triglav::io {

class Deserializer
{
 public:
   explicit Deserializer(IReader& reader);

#define TG_IO_TYPE(TYPE, FUNC, WFUNC) TYPE FUNC();
   TG_IO_SERIALIZATION_TYPES
#undef TG_IO_TYPE

   std::string read_string();

   template<typename T>
   [[nodiscard]] T read_value()
   {
      if constexpr (std::is_same_v<T, std::string>)
         return this->read_string();
#define TG_IO_TYPE(TYPE, FUNC, WFUNC) else if constexpr (std::is_same_v<T, TYPE>) return this->FUNC();
      TG_IO_SERIALIZATION_TYPES
#undef TG_IO_TYPE
      else return T{};
   }

 private:
   template<typename T>
   [[nodiscard]] T read_value_internal();

   IReader& m_reader;
};

}// namespace triglav::io
