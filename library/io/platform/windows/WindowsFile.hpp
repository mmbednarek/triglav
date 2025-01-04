#pragma once

#include "File.hpp"

#include <windows.h>

namespace triglav::io::windows {

class WindowsFile final : public IFile
{
 public:
   explicit WindowsFile(HFILE file);
   ~WindowsFile() override;

   [[nodiscard]] Result<MemorySize> read(std::span<u8> buffer) override;
   [[nodiscard]] Result<MemorySize> write(std::span<const u8> buffer) override;
   [[nodiscard]] Status seek(SeekPosition position, MemoryOffset offset) override;
   [[nodiscard]] Result<MemorySize> file_size() override;

 private:
   HFILE m_file;
};

}// namespace triglav::io::windows
