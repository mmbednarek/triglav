#pragma once

#include "Result.hpp"

#include "triglav/Int.hpp"

#include <span>

namespace triglav::io {

class IStreamBase
{
 public:
   virtual ~IStreamBase() = default;
};

class IReader : virtual public IStreamBase
{
 public:
   virtual Result<MemorySize> read(std::span<u8> buffer) = 0;
};

class IWriter : virtual public IStreamBase
{
 public:
   virtual Result<MemorySize> write(std::span<const u8> buffer) = 0;
};

class IStream : public IReader, public IWriter
{};

enum class SeekPosition
{
   Begin,
   Current,
   End,
};

class ISeekableStream : public IStream
{
 public:
   virtual Status seek(SeekPosition position, MemoryOffset offset) = 0;
};


}// namespace triglav::io