#pragma once

#include "triglav/Macros.hpp"

#include <string_view>

namespace triglav::project {

std::string_view project_name();

}

#define TG_PROJECT_NAME(name)                    \
   namespace triglav::project {                  \
   [[nodiscard]] std::string_view project_name() \
   {                                             \
      return TG_STRING(name);                    \
   }                                             \
   }
