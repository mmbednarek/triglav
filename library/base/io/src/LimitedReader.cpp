#include "LimitedReader.hpp"

namespace triglav::io {

LimitedReader::LimitedReader(IReader& reader, const MemorySize limit) :
    m_reader(reader),
    m_limit(limit)
{
}

Result<MemorySize> LimitedReader::read(std::span<u8> buffer)
{
   if (m_offset >= m_limit) {
      return 0;
   }

   auto result = m_reader.read({buffer.data(), std::min(buffer.size(), m_limit - m_offset)});
   if (!result.has_value()) {
      return result;
   }

   m_offset += *result;
   return result;
}

}// namespace triglav::io
