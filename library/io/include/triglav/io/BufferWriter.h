#pragma once

#include "Stream.h"

#include "triglav/Int.hpp"

namespace triglav::io {

class BufferWriter : public IWriter
{
 public:
   explicit BufferWriter(std::span<u8> buffer);

   Result<MemorySize> write(std::span<u8> buffer) override;
   [[nodiscard]] u32 offset() const;

 private:
   std::span<u8> m_buffer;
   u32 m_offset{};
};

}// namespace triglav::io