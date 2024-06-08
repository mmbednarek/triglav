#pragma once

#include "WindowsFile.h"

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
   }

   return 0;
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
   CloseHandle(reinterpret_cast<HANDLE>(m_file));
}

Result<MemorySize> WindowsFile::read(const std::span<u8> buffer)
{
   DWORD bytesRead{};
   const auto res = ReadFile(reinterpret_cast<HANDLE>(m_file), buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr);
   if (not res) {
      return std::unexpected(Status::BrokenPipe);
   }
   return static_cast<MemorySize>(bytesRead);
}

Result<MemorySize> WindowsFile::write(const std::span<u8> buffer)
{
   DWORD bytesWritten{};
   const auto res = WriteFile(reinterpret_cast<HANDLE>(m_file), buffer.data(), static_cast<DWORD>(buffer.size()), &bytesWritten, nullptr);
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

   const auto res = SetFilePointer(reinterpret_cast<HANDLE>(m_file), static_cast<LONG>(offset), nullptr, moveMethod);
   if (res == INVALID_SET_FILE_POINTER) {
      return Status::BrokenPipe;
   }

   return Status::Success;
}

Result<MemorySize> WindowsFile::file_size()
{
   DWORD fileSizeHigh{};
   const auto res = GetFileSize(reinterpret_cast<HANDLE>(m_file), &fileSizeHigh);
   if (res == INVALID_FILE_SIZE) {
      return std::unexpected(Status::InvalidFile);
   }

   if (fileSizeHigh == 0) {
      return static_cast<MemorySize>(res);
   }

   return static_cast<MemorySize>(fileSizeHigh) << 32 | static_cast<MemorySize>(res);
}

}// namespace triglav::io::windows
