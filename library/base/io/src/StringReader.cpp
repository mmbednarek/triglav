#include "StringReader.hpp"

#include <cstring>

namespace triglav::io {

StringReader::StringReader(const std::string_view string_ref) :
    m_string_ref(string_ref)
{
}

Result<MemorySize> StringReader::read(const std::span<u8> buffer)
{
   const auto read_size = std::min(buffer.size(), m_string_ref.size() - m_offset);
   if (read_size == 0) {
      return 0;
   }

   std::memcpy(buffer.data(), m_string_ref.data() + m_offset, read_size);
   m_offset += static_cast<MemoryOffset>(read_size);
   return read_size;
}

}// namespace triglav::io