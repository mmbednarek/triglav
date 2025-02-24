#pragma once

#include "File.hpp"

namespace triglav::io::linux {

class UnixFile final : public IFile
{
 public:
   explicit UnixFile(int fileDescriptor, std::string&& filePath);
   ~UnixFile() override;

   [[nodiscard]] Result<MemorySize> read(std::span<u8> buffer) override;
   [[nodiscard]] Result<MemorySize> write(std::span<const u8> buffer) override;
   [[nodiscard]] Status seek(SeekPosition position, MemoryOffset offset) override;
   [[nodiscard]] Result<MemorySize> file_size() override;
   [[nodiscard]] MemorySize position() const override;

 private:
   int m_fileDescriptor;
   std::string m_filePath;
};

}// namespace triglav::io::linux