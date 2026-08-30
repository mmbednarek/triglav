#pragma once

#include "Stream.hpp"

#include <span>

namespace triglav::io {


class BufferReader : public IReader
{
public:
   explicit BufferReader(std::span<const u8> buffer);

  Result<MemorySize> read(std::span<u8> buffer) override;

private:
   std::span<const u8> m_buffer;
   mem_size m_read_offset{0};
};



}