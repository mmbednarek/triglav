#include "UnixFile.hpp"

#include <cassert>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace triglav::io {

Result<IFileUPtr> open_file(const io::Path& path, const FileModeFlags mode)
{
   int flags{};
   if (mode & FileMode::Read && mode & FileMode::Write) {
      flags |= O_RDWR;
   } else if (mode & FileMode::Read) {
      flags |= O_RDONLY;
   } else if (mode & FileMode::Write) {
      flags |= O_WRONLY | O_TRUNC;
   }

   if (mode & FileMode::Create) {
      flags |= O_CREAT;
   }
   if (mode & FileMode::Append) {
      flags |= O_APPEND;
   }

   const auto res = ::open(path.string().c_str(), flags, 0644);
   if (res < 0) {
      return std::unexpected{Status::InvalidFile};
   }

   return std::make_unique<linux::UnixFile>(res, std::string{path.string()});
}

}// namespace triglav::io

namespace triglav::io::linux {

UnixFile::UnixFile(int file_descriptor, std::string&& file_path) :
    m_file_descriptor(file_descriptor),
    m_file_path(std::move(file_path))
{
}

UnixFile::~UnixFile()
{
   ::close(m_file_descriptor);
}

Result<MemorySize> UnixFile::read(const std::span<u8> buffer)
{
   const auto result = ::read(m_file_descriptor, buffer.data(), buffer.size());
   if (result < 0)
      return std::unexpected(Status::BrokenPipe);
   return static_cast<MemorySize>(result);
}

Result<MemorySize> UnixFile::write(const std::span<const u8> buffer)
{
   const auto result = ::write(m_file_descriptor, buffer.data(), buffer.size());
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

   const auto res = ::lseek(m_file_descriptor, offset, whence);
   if (res < 0)
      return Status::BrokenPipe;

   return Status::Success;
}

Result<MemorySize> UnixFile::file_size()
{
   struct ::stat file_stat{};

   const auto res = ::stat(m_file_path.c_str(), &file_stat);
   if (res < 0) {
      return std::unexpected{Status::InvalidFile};
   }
   return file_stat.st_size;
}

MemorySize UnixFile::position() const
{
   const auto res = ::lseek(m_file_descriptor, 0, SEEK_CUR);
   assert(res >= 0);
   return res;
}

}// namespace triglav::io::linux