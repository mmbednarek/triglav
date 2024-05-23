#include "Path.h"

#include <unistd.h>

namespace triglav::io {

[[nodiscard]] Result<Path> working_path()
{
   char buffer[1024];
   auto* result = ::getcwd(buffer, 1024);
   if (result == nullptr) {
      return std::unexpected{Status::InvalidDirectory};
   }

   return Path{buffer};
}

}
