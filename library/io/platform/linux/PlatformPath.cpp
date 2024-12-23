#include "Path.hpp"

#include <climits>
#include <unistd.h>

namespace triglav::io {

Result<Path> working_path()
{
   char buffer[1024];
   auto* result = ::getcwd(buffer, 1024);
   if (result == nullptr) {
      return std::unexpected{Status::InvalidDirectory};
   }

   return Path{buffer};
}

bool is_existing_path(const Path& path)
{
   return ::access(path.string().c_str(), F_OK) == 0;
}

Result<std::string> full_path(const std::string_view path)
{
   char result[PATH_MAX];
   ::realpath(path.data(), result);
   return std::string{result};
}

}// namespace triglav::io
