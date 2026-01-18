#include "Path.hpp"

#include <cstring>
#include <expected>
#include <format>
#include <shlobj.h>
#include <windows.h>

namespace triglav::io {

constexpr auto g_max_path = 4096;

Result<Path> working_path()
{
   char result[g_max_path];
   auto count = GetCurrentDirectoryA(g_max_path, result);
   return Path{std::string{result, count}};
}

bool is_existing_path(const Path& path)
{
   return GetFileAttributesA(path.string().data()) != INVALID_FILE_ATTRIBUTES;
}

Result<std::string> full_path(const std::string_view path)
{
   char result[g_max_path];
   const auto count = GetFullPathNameA(path.data(), g_max_path, result, nullptr);
   return std::string{result, count};
}

Result<Path> home_directory()
{
   ::PWSTR home_dir;
   const auto status = ::SHGetKnownFolderPath(FOLDERID_Profile, KF_FLAG_DEFAULT, nullptr, &home_dir);
   if (status != S_OK) {
      return std::unexpected{Status::InvalidDirectory};
   }

   const auto byte_count = ::WideCharToMultiByte(CP_UTF8, 0, home_dir, -1, nullptr, 0, nullptr, nullptr);
   if (byte_count == 0) {
      return std::unexpected{Status::InvalidDirectory};
   }

   std::string home_dir_str(byte_count, '\0');
   ::WideCharToMultiByte(CP_UTF8, 0, home_dir, -1, home_dir_str.data(), static_cast<int>(home_dir_str.size()), nullptr, nullptr);

   return Path{home_dir_str};
}

std::string sub_directory(const std::string_view path, const std::string_view directory)
{
   std::string res(path.size() + directory.size() + 1, ' ');
   std::memcpy(res.data(), path.data(), path.size());
   res[path.size()] = '\\';
   std::memcpy(res.data() + path.size() + 1, directory.data(), directory.size());
   for (size_t i = path.size() + 1; i < res.size(); ++i) {
      if (res[i] == '/') {
         res[i] = '\\';
      }
   }
   return res;
}

char path_seperator()
{
   return '\\';
}

[[nodiscard]] bool make_directory(const Path& path)
{
   return ::CreateDirectory(path.string().data(), nullptr);
}

}// namespace triglav::io
