#include "BufferWriter.h"

#include <cassert>
#include <cstring>

namespace triglav::io {

BufferWriter::BufferWriter(std::span<u8> buffer) :
    m_buffer(buffer)
{
}

Result<MemorySize> BufferWriter::write(const std::span<const u8> buffer)
{
   if ((m_offset + buffer.size()) > m_buffer.size()) {
      return std::unexpected{Status::BufferTooSmall};
   }

   std::memcpy(m_buffer.data() + m_offset, buffer.data(), buffer.size());
   m_offset += buffer.size();
   return buffer.size();
}

u32 BufferWriter::offset() const
{
   return m_offset;
}

}// namespace triglav::io