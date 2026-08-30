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
   [[maybe_unused]] const auto read_res = m_reader.read({reinterpret_cast<u8*>(&result), sizeof(T)});
   assert(read_res.has_value());
   return result;
}

std::string Deserializer::read_string()
{
   const u32 count = this->read_u32();
   std::string out(count, ' ');
   const auto result = m_reader.read({reinterpret_cast<u8*>(out.data()), out.size()});
   if (!result.has_value())
      return {};
   return out;
}

#define TG_IO_TYPE(TYPE, FUNC, WFUNC)           \
   TYPE Deserializer::FUNC()                    \
   {                                            \
      return this->read_value_internal<TYPE>(); \
   }
TG_IO_SERIALIZATION_TYPES
#undef TG_IO_TYPE

}// namespace triglav::io
