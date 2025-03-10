#pragma once

#include "triglav/io/Path.hpp"

namespace triglav::tool::cli {

struct LevelImportProps
{
   io::Path srcPath;
   io::Path dstPath;
   bool shouldOverride{};
};

[[nodiscard]] bool import_level(const LevelImportProps& props);

}
