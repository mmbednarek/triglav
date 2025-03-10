#include "DisplacedStream.hpp"

namespace triglav::io {

DisplacedStream::DisplacedStream(ISeekableStream& stream, const MemoryOffset offset, MemorySize size) :
    m_stream(stream),
    m_offset(offset),
    m_size(size)
{
}

Result<MemorySize> DisplacedStream::read(const std::span<u8> buffer)
{
   const auto postReadPos = this->position() + buffer.size();
   if (postReadPos > m_size) {
      return m_stream.read({buffer.data(), m_size - this->position()});
   }
   return m_stream.read(buffer);
}

Result<MemorySize> DisplacedStream::write(const std::span<const u8> buffer)
{
   const auto postWritePos = this->position() + buffer.size();
   if (postWritePos > m_size) {
      return m_stream.write({buffer.data(), m_size - this->position()});
   }
   return m_stream.write(buffer);
}

Status DisplacedStream::seek(const SeekPosition position, const MemoryOffset offset)
{
   if (position == SeekPosition::End) {
      if (offset > 0) {
         return m_stream.seek(SeekPosition::Begin, m_offset + m_size);
      }
      return m_stream.seek(SeekPosition::Begin, m_offset + m_size + offset);
   }
   if (position == SeekPosition::Begin) {
      if (offset > static_cast<MemoryOffset>(m_size)) {
         return m_stream.seek(SeekPosition::Begin, m_offset + m_size);
      }
      return m_stream.seek(SeekPosition::Begin, m_offset + offset);
   }
   if (position == SeekPosition::Current) {
      if ((this->position() + offset) > m_size) {
         return m_stream.seek(SeekPosition::Begin, m_offset + m_size);
      }
      return m_stream.seek(SeekPosition::Current, offset);
   }
   return Status::BrokenPipe;
}

MemorySize DisplacedStream::position() const
{
   return m_stream.position() - m_offset;
}

}// namespace triglav::io