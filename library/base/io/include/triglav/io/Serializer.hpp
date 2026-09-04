#pragma once

#include "Deserializer.hpp"
#include "Stream.hpp"

namespace triglav::io {

class Serializer
{
 public:
   explicit Serializer(IWriter& writer);

#define TG_IO_TYPE(TYPE, RFUNC, FUNC) Result<mem_size> FUNC(const TYPE& value);
   TG_IO_SERIALIZATION_TYPES
#undef TG_IO_TYPE

   Result<mem_size> write_string(std::string_view value);

   template<typename T>
   [[nodiscard]] Result<mem_size> write_value(const T& value)
   {
      if (std::is_same_v<T, std::string>)
         return this->write_string(value);
#define TG_IO_TYPE(TYPE, RFUNC, FUNC) else if constexpr (std::is_same_v<T, TYPE>) return this->FUNC(value);
      TG_IO_SERIALIZATION_TYPES
#undef TG_IO_TYPE
      else return std::unexpected(Status::SerializationError);
   }

 private:
   template<typename T>
   Result<mem_size> write_value_internal(const T& value);

   IWriter& m_writer;
};

}// namespace triglav::io