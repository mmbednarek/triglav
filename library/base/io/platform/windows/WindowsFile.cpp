#include "WindowsFile.hpp"

#include <cassert>
#include <expected>

namespace triglav::io {

namespace {

UINT map_file_open_style(const FileOpenMode mode)
{
   switch (mode) {
   case FileOpenMode::Read:
      return OF_READ;
   case FileOpenMode::Write:
      return OF_WRITE;
   case FileOpenMode::ReadWrite:
      return OF_READ | OF_WRITE;
   case FileOpenMode::Create:
      return OF_WRITE | OF_CREATE;
   }

   return 0;
}


HANDLE hfile_to_handle(HFILE file)
{
   return reinterpret_cast<HANDLE>(static_cast<u64>(file));
}

}// namespace

Result<IFileUPtr> open_file(const Path& path, const FileOpenMode mode)
{
   OFSTRUCT openFileInfo{};
   const auto file = OpenFile(path.string().data(), &openFileInfo, map_file_open_style(mode));
   if (file == HFILE_ERROR) {
      return std::unexpected(Status::BrokenPipe);
   }

   return std::make_unique<windows::WindowsFile>(file);
}

}// namespace triglav::io

namespace triglav::io::windows {

WindowsFile::WindowsFile(const HFILE file) :
    m_file(file)
{
}

WindowsFile::~WindowsFile()
{
   CloseHandle(hfile_to_handle(m_file));
}

Result<MemorySize> WindowsFile::read(const std::span<u8> buffer)
{
   DWORD bytesRead{};
   const auto res = ReadFile(hfile_to_handle(m_file), buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr);
   if (not res) {
      return std::unexpected(Status::BrokenPipe);
   }
   return static_cast<MemorySize>(bytesRead);
}

Result<MemorySize> WindowsFile::write(const std::span<const u8> buffer)
{
   DWORD bytesWritten{};
   const auto res = WriteFile(hfile_to_handle(m_file), buffer.data(), static_cast<DWORD>(buffer.size()), &bytesWritten, nullptr);
   if (not res) {
      return std::unexpected(Status::BrokenPipe);
   }
   return static_cast<MemorySize>(bytesWritten);
}

Status WindowsFile::seek(const SeekPosition position, const MemoryOffset offset)
{
   DWORD moveMethod{};
   switch (position) {
   case SeekPosition::Begin:
      moveMethod = FILE_BEGIN;
      break;
   case SeekPosition::Current:
      moveMethod = FILE_CURRENT;
      break;
   case SeekPosition::End:
      moveMethod = FILE_END;
      break;
   }

   const auto res = SetFilePointer(hfile_to_handle(m_file), static_cast<LONG>(offset), nullptr, moveMethod);
   if (res == INVALID_SET_FILE_POINTER) {
      return Status::BrokenPipe;
   }

   return Status::Success;
}

Result<MemorySize> WindowsFile::file_size()
{
   DWORD fileSizeHigh{};
   const auto res = GetFileSize(hfile_to_handle(m_file), &fileSizeHigh);
   if (res == INVALID_FILE_SIZE) {
      return std::unexpected(Status::InvalidFile);
   }

   if (fileSizeHigh == 0) {
      return static_cast<MemorySize>(res);
   }

   return static_cast<MemorySize>(fileSizeHigh) << 32 | static_cast<MemorySize>(res);
}

MemorySize WindowsFile::position() const
{
   const auto res = SetFilePointer(hfile_to_handle(m_file), 0, nullptr, FILE_CURRENT);
   assert(res != INVALID_SET_FILE_POINTER);
   return res;
}

}// namespace triglav::io::windows
