#include "Path.h"

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

}
