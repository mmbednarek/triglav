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
   [[nodiscard]] virtual MemorySize position() const = 0;
};

class IConsoleWriter : public IWriter
{
 public:
   [[nodiscard]] virtual bool supports_color_output() const = 0;
};

[[nodiscard]] IConsoleWriter& stdout_writer();
[[nodiscard]] IConsoleWriter& stderr_writer();
[[nodiscard]] IReader& stdin_reader();

}// namespace triglav::io