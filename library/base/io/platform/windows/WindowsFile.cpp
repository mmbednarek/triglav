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
   OFSTRUCT open_file_info{};
   const auto file = OpenFile(path.string().data(), &open_file_info, map_file_open_style(mode));
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
   DWORD bytes_read{};
   const auto res = ReadFile(hfile_to_handle(m_file), buffer.data(), static_cast<DWORD>(buffer.size()), &bytes_read, nullptr);
   if (not res) {
      return std::unexpected(Status::BrokenPipe);
   }
   return static_cast<MemorySize>(bytes_read);
}

Result<MemorySize> WindowsFile::write(const std::span<const u8> buffer)
{
   DWORD bytes_written{};
   const auto res = WriteFile(hfile_to_handle(m_file), buffer.data(), static_cast<DWORD>(buffer.size()), &bytes_written, nullptr);
   if (not res) {
      return std::unexpected(Status::BrokenPipe);
   }
   return static_cast<MemorySize>(bytes_written);
}

Status WindowsFile::seek(const SeekPosition position, const MemoryOffset offset)
{
   DWORD move_method{};
   switch (position) {
   case SeekPosition::Begin:
      move_method = FILE_BEGIN;
      break;
   case SeekPosition::Current:
      move_method = FILE_CURRENT;
      break;
   case SeekPosition::End:
      move_method = FILE_END;
      break;
   }

   const auto res = SetFilePointer(hfile_to_handle(m_file), static_cast<LONG>(offset), nullptr, move_method);
   if (res == INVALID_SET_FILE_POINTER) {
      return Status::BrokenPipe;
   }

   return Status::Success;
}

Result<MemorySize> WindowsFile::file_size()
{
   DWORD file_size_high{};
   const auto res = GetFileSize(hfile_to_handle(m_file), &file_size_high);
   if (res == INVALID_FILE_SIZE) {
      return std::unexpected(Status::InvalidFile);
   }

   if (file_size_high == 0) {
      return static_cast<MemorySize>(res);
   }

   return static_cast<MemorySize>(file_size_high) << 32 | static_cast<MemorySize>(res);
}

MemorySize WindowsFile::position() const
{
   const auto res = SetFilePointer(hfile_to_handle(m_file), 0, nullptr, FILE_CURRENT);
   assert(res != INVALID_SET_FILE_POINTER);
   return res;
}

}// namespace triglav::io::windows
