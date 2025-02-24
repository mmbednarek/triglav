#include "DisplacedStream.hpp"

namespace triglav::io {

DisplacedStream::DisplacedStream(ISeekableStream& stream, const MemoryOffset offset) :
    m_stream(stream),
    m_offset(offset)
{
}

Result<MemorySize> DisplacedStream::read(const std::span<u8> buffer)
{
   return m_stream.read(buffer);
}

Result<MemorySize> DisplacedStream::write(const std::span<const u8> buffer)
{
   return m_stream.write(buffer);
}

Status DisplacedStream::seek(const SeekPosition position, const MemoryOffset offset)
{
   return m_stream.seek(position, m_offset + offset);
}

MemorySize DisplacedStream::position() const
{
   return m_stream.position() - m_offset;
}

}// namespace triglav::io