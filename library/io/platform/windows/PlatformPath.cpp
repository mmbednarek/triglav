#include "Path.hpp"

#include <fmt/core.h>
#include <shlobj.h>
#include <windows.h>

namespace triglav::io {

constexpr auto g_maxPath = 4096;

Result<Path> working_path()
{
   char result[g_maxPath];
   auto count = GetCurrentDirectoryA(g_maxPath, result);
   return Path{std::string{result, count}};
}

bool is_existing_path(const Path& path)
{
   return GetFileAttributesA(path.string().c_str()) != INVALID_FILE_ATTRIBUTES;
}

Result<std::string> full_path(const std::string_view path)
{
   char result[g_maxPath];
   const auto count = GetFullPathNameA(path.data(), g_maxPath, result, nullptr);
   return std::string{result, count};
}

Result<Path> home_directory()
{
   ::PWSTR homeDir;
   const auto status = ::SHGetKnownFolderPath(FOLDERID_Profile, KF_FLAG_DEFAULT, nullptr, &homeDir);
   if (status != S_OK) {
      return std::unexpected{Status::InvalidDirectory};
   }

   const auto byteCount = ::WideCharToMultiByte(CP_UTF8, 0, homeDir, -1, nullptr, 0, nullptr, nullptr);
   if (byteCount == 0) {
      return std::unexpected{Status::InvalidDirectory};
   }

   std::string homeDirStr(byteCount, '\0');
   ::WideCharToMultiByte(CP_UTF8, 0, homeDir, -1, homeDirStr.data(), homeDirStr.size(), nullptr, nullptr);

   return Path{homeDirStr};
}

std::string sub_directory(const std::string_view path, const std::string_view directory)
{
   return fmt::format("{}\\{}", path, directory);
}

char path_seperator()
{
   return '\\';
}

}// namespace triglav::io
