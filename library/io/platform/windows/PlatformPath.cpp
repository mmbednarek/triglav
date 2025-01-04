#include "Path.hpp"

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

}// namespace triglav::io
