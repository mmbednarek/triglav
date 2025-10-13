#include "Deserializer.hpp"

#include <cassert>

namespace triglav::io {

Deserializer::Deserializer(IReader& reader) :
    m_reader(reader)
{
}

template<typename T>
T Deserializer::read_value_internal()
{
   T result{};
   [[maybe_unused]] const auto readRes = m_reader.read({reinterpret_cast<u8*>(&result), sizeof(T)});
   assert(readRes.has_value());
   return result;
}

#define TG_IO_TYPE(TYPE)                        \
   TYPE Deserializer::read_##TYPE()             \
   {                                            \
      return this->read_value_internal<TYPE>(); \
   }
TG_IO_DESERIALIZER_TYPES
#undef TG_IO_TYPE

}// namespace triglav::io
