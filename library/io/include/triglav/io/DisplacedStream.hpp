#pragma once

#include "Stream.hpp"

namespace triglav::io {

class DisplacedStream final : public ISeekableStream
{
 public:
   DisplacedStream(ISeekableStream& stream, MemoryOffset offset);

   Result<MemorySize> read(std::span<u8> buffer) override;
   Result<MemorySize> write(std::span<const u8> buffer) override;
   Status seek(SeekPosition position, MemoryOffset offset) override;
   [[nodiscard]] MemorySize position() const override;

 private:
   ISeekableStream& m_stream;
   MemoryOffset m_offset;
};

}// namespace triglav::io