#include "Stream.hpp"

extern "C"
{
#include <unistd.h>
}

namespace triglav::io {

class StandardStream final : public IReader, public IConsoleWriter
{
 public:
   explicit StandardStream(const i32 fd) :
       m_file_descriptor(fd)
   {
   }

   Result<MemorySize> read(const std::span<u8> buffer) override
   {
      const ssize_t res = ::read(m_file_descriptor, buffer.data(), buffer.size());
      return res;
   }

   Result<MemorySize> write(const std::span<const u8> buffer) override
   {
      const ssize_t res = ::write(m_file_descriptor, buffer.data(), buffer.size());
      return res;
   }

   [[nodiscard]] bool supports_color_output() const override
   {
      return ::isatty(m_file_descriptor) != 0;
   }

 private:
   i32 m_file_descriptor{};
};

[[nodiscard]] IConsoleWriter& stdout_writer()
{
   static StandardStream stream{STDOUT_FILENO};
   return stream;
}

[[nodiscard]] IConsoleWriter& stderr_writer()
{
   static StandardStream stream{STDERR_FILENO};
   return stream;
}

[[nodiscard]] IReader& stdin_reader()
{
   static StandardStream stream{STDIN_FILENO};
   return stream;
}

MemorySize copy(IReader& input, IWriter& output)
{
   static constexpr auto copy_buffer_size = 4096ull;
   std::array<u8, copy_buffer_size> copy_buffer{};

   MemorySize total_copied{};
   for (;;) {
      const auto read_size = input.read(copy_buffer);
      if (!read_size.has_value()) {
         return 0;
      }
      if (*read_size == 0) {
         return total_copied;
      }

      output.write({copy_buffer.data(), *read_size});
      total_copied += *read_size;
      if (*read_size < copy_buffer_size) {
         return total_copied;
      }
   }
}

}// namespace triglav::io