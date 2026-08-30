#include "BufferReader.hpp"

#include <cstring>

namespace triglav::io {

BufferReader::BufferReader(const std::span<const u8> buffer) :
    m_buffer(buffer)
{
}

Result<MemorySize> BufferReader::read(const std::span<u8> buffer)
{
   const mem_size read_size = std::min(buffer.size(), m_buffer.size() - m_read_offset);
   std::memcpy(buffer.data(), m_buffer.data() + m_read_offset, read_size);
   m_read_offset += read_size;
   return read_size;
}

}// namespace triglav::io
