#pragma once

#include "Stream.hpp"

#include "triglav/Math.hpp"

#include <type_traits>

#define TG_IO_DESERIALIZER_TYPES \
   TG_IO_TYPE(Vector2)           \
   TG_IO_TYPE(Vector3)           \
   TG_IO_TYPE(Vector4)           \
   TG_IO_TYPE(MemorySize)        \
   TG_IO_TYPE(u32)               \
   TG_IO_TYPE(u16)

namespace triglav::io {

class Deserializer
{
 public:
   explicit Deserializer(IReader& reader);

#define TG_IO_TYPE(TYPE) TYPE read_##TYPE();
   TG_IO_DESERIALIZER_TYPES
#undef TG_IO_TYPE

   template<typename T>
   [[nodiscard]] T read_value()
   {
      if (false)
         ;
#define TG_IO_TYPE(TYPE) else if constexpr (std::is_same_v<T, TYPE>) return this->read_##TYPE();
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
