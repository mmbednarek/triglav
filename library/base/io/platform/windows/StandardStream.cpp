#include "Stream.hpp"

#include <windows.h>

namespace triglav::io {

class StandardStream final : public IReader, public IConsoleWriter
{
 public:
   explicit StandardStream(const ::HANDLE handle) :
       m_handle(handle)
   {
   }

   Result<MemorySize> read(const std::span<u8> buffer) override
   {
      DWORD bytes_read{};
      const auto res = ::ReadConsole(m_handle, buffer.data(), static_cast<DWORD>(buffer.size()), &bytes_read, nullptr);
      if (not res) {
         return std::unexpected(Status::BrokenPipe);
      }
      return static_cast<MemorySize>(bytes_read);
   }

   Result<MemorySize> write(const std::span<const u8> buffer) override
   {
      DWORD bytes_written{};
      const BOOL res = ::WriteConsole(m_handle, buffer.data(), static_cast<DWORD>(buffer.size()), &bytes_written, nullptr);
      if (!res) {
         return std::unexpected(Status::BrokenPipe);
      }
      return res;
   }

   [[nodiscard]] bool supports_color_output() const override
   {
      DWORD mode = 0;
      if (!::GetConsoleMode(m_handle, &mode)) {
         return false;
      }

      return (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
   }

 private:
   ::HANDLE m_handle;
};

[[nodiscard]] IConsoleWriter& stdout_writer()
{
   static StandardStream stream{::GetStdHandle(STD_OUTPUT_HANDLE)};
   return stream;
}

[[nodiscard]] IConsoleWriter& stderr_writer()
{
   static StandardStream stream{::GetStdHandle(STD_ERROR_HANDLE)};
   return stream;
}

[[nodiscard]] IReader& stdin_reader()
{
   static StandardStream stream{::GetStdHandle(STD_INPUT_HANDLE)};
   return stream;
}

}// namespace triglav::io
