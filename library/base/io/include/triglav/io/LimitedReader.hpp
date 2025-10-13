#pragma once

#include "Stream.hpp"

namespace triglav::io {

class LimitedReader : public IReader
{
 public:
   explicit LimitedReader(IReader& reader, MemorySize limit);

   Result<MemorySize> read(std::span<u8> buffer) override;

 private:
   IReader& m_reader;
   MemorySize m_limit;
   MemorySize m_offset{};
};

}// namespace triglav::io