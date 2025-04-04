#include "UnixFile.hpp"

#include <cassert>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace triglav::io {

Result<IFileUPtr> open_file(const io::Path& path, const FileOpenMode mode)
{
   int flags{};
   switch (mode) {
   case FileOpenMode::Read:
      flags = O_RDONLY;
      break;
   case FileOpenMode::Write:
      flags = O_WRONLY | O_TRUNC;
      break;
   case FileOpenMode::ReadWrite:
      flags = O_RDWR;
      break;
   case FileOpenMode::Create:
      flags = O_WRONLY | O_CREAT | O_TRUNC;
      break;
   }

   const auto res = ::open(path.string().c_str(), flags, 0644);
   if (res < 0) {
      return std::unexpected{Status::InvalidFile};
   }

   return std::make_unique<linux::UnixFile>(res, std::string{path.string()});
}

}// namespace triglav::io

namespace triglav::io::linux {

UnixFile::UnixFile(int fileDescriptor, std::string&& filePath) :
    m_fileDescriptor(fileDescriptor),
    m_filePath(std::move(filePath))
{
}

UnixFile::~UnixFile()
{
   ::close(m_fileDescriptor);
}

Result<MemorySize> UnixFile::read(const std::span<u8> buffer)
{
   const auto result = ::read(m_fileDescriptor, buffer.data(), buffer.size());
   if (result < 0)
      return std::unexpected(Status::BrokenPipe);
   return static_cast<MemorySize>(result);
}

Result<MemorySize> UnixFile::write(const std::span<const u8> buffer)
{
   const auto result = ::write(m_fileDescriptor, buffer.data(), buffer.size());
   if (result < 0)
      return std::unexpected(Status::BrokenPipe);
   return static_cast<MemorySize>(result);
}

Status UnixFile::seek(const SeekPosition position, const MemoryOffset offset)
{
   int whence{};
   switch (position) {
   case SeekPosition::Begin:
      whence = SEEK_SET;
      break;
   case SeekPosition::Current:
      whence = SEEK_CUR;
      break;
   case SeekPosition::End:
      whence = SEEK_END;
      break;
   }

   const auto res = ::lseek(m_fileDescriptor, offset, whence);
   if (res < 0)
      return Status::BrokenPipe;

   return Status::Success;
}

Result<MemorySize> UnixFile::file_size()
{
   struct ::stat fileStat{};

   const auto res = ::stat(m_filePath.c_str(), &fileStat);
   if (res < 0) {
      return std::unexpected{Status::InvalidFile};
   }
   return fileStat.st_size;
}

MemorySize UnixFile::position() const
{
   const auto res = ::lseek(m_fileDescriptor, 0, SEEK_CUR);
   assert(res >= 0);
   return res;
}

}// namespace triglav::io::linux