#include "StringReader.hpp"

#include <cstring>

namespace triglav::io {

StringReader::StringReader(const std::string_view stringRef) :
    m_stringRef(stringRef)
{
}

Result<MemorySize> StringReader::read(const std::span<u8> buffer)
{
   const auto readSize = std::min(buffer.size(), m_stringRef.size() - m_offset);
   if (readSize == 0) {
      return 0;
   }

   std::memcpy(buffer.data(), m_stringRef.data() + m_offset, readSize);
   m_offset += static_cast<MemoryOffset>(readSize);
   return readSize;
}

}// namespace triglav::io