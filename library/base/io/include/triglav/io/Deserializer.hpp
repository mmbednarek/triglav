#pragma once

#include "Stream.hpp"

#include "triglav/Math.hpp"

#include <type_traits>

#define TG_IO_DESERIALIZER_TYPES         \
   TG_IO_TYPE(Vector2, read_vec2)        \
   TG_IO_TYPE(Vector3, read_vec3)        \
   TG_IO_TYPE(Vector4, read_vec4)        \
   TG_IO_TYPE(MemorySize, read_mem_size) \
   TG_IO_TYPE(u32, read_u32)             \
   TG_IO_TYPE(u16, read_u16)             \
   TG_IO_TYPE(float, read_float)

namespace triglav::io {

class Deserializer
{
 public:
   explicit Deserializer(IReader& reader);

#define TG_IO_TYPE(TYPE, FUNC) TYPE FUNC();
   TG_IO_DESERIALIZER_TYPES
#undef TG_IO_TYPE

   template<typename T>
   [[nodiscard]] T read_value()
   {
      if (false)
         ;
#define TG_IO_TYPE(TYPE, FUNC) else if constexpr (std::is_same_v<T, TYPE>) return this->FUNC();
      TG_IO_DESERIALIZER_TYPES
#undef TG_IO_TYPE
      else return T{};
   }

 private:
   template<typename T>
   [[nodiscard]] T read_value_internal();

   IReader& m_reader;
};

}// namespace triglav::io
