#include "Serializer.hpp"

namespace triglav::io {

Serializer::Serializer(IWriter& writer) :
    m_writer(writer)
{
}

template<typename T>
Result<mem_size> Serializer::write_value_internal(const T& value)
{
   return m_writer.write({reinterpret_cast<const u8*>(&value), sizeof(T)});
}

Result<mem_size> Serializer::write_string(const std::string_view value)
{
   const auto res = this->write_u32(value.size());
   if (!res.has_value()) {
      return res;
   }

   return m_writer.write({reinterpret_cast<const u8*>(value.data()), value.size()});
}

#define TG_IO_TYPE(TYPE, RFUNC, FUNC)                   \
   Result<mem_size> Serializer::FUNC(const TYPE& value) \
   {                                                    \
      return this->write_value_internal<TYPE>(value);   \
   }
TG_IO_SERIALIZATION_TYPES
#undef TG_IO_TYPE

}// namespace triglav::io
