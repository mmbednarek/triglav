#include "UnixFile.hpp"

#include <cassert>
extern "C"
{
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
}

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

   const auto res = ::open(path.string().data(), flags, 0644);
   if (res < 0) {
      return std::unexpected{Status::InvalidFile};
   }

   return std::make_unique<UnixFile>(res, std::string{path.string()});
}

union UnixDirectoryStream
{
   DirectoryStream stream;
   ::DIR* unix_handle;
};
static_assert(sizeof(UnixDirectoryStream) == sizeof(DirectoryStream));

Result<DirectoryStream> DirectoryStream::create(const Path& path)
{
   UnixDirectoryStream stream{};
   stream.unix_handle = ::opendir(path.string().data());
   if (stream.unix_handle == nullptr) {
      return std::unexpected{Status::InvalidDirectory};
   }
   return std::bit_cast<DirectoryStream>(stream);
}

[[nodiscard]] std::optional<ListedFile> DirectoryStream::next()
{
   const auto* stream = std::bit_cast<UnixDirectoryStream*>(this);
   const auto* directory = ::readdir(stream->unix_handle);
   if (directory == nullptr)
      return std::nullopt;
   if (std::strcmp(directory->d_name, ".") == 0 || std::strcmp(directory->d_name, "..") == 0) {
      return this->next();
   }
   return ListedFile{StringView{directory->d_name}, (directory->d_type & DT_DIR) != 0};
}

}// namespace triglav::io

namespace triglav::io {

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

Status remove_file(const Path& path)
{
   if (::unlink(path.string().data()) != 0)
      return Status::Success;
   return Status::InvalidFile;
}

}// namespace triglav::io