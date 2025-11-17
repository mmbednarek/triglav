#pragma once

#include "triglav/io/Path.hpp"

namespace triglav::tool::cli {

struct LevelImportProps
{
   io::Path src_path;
   io::Path dst_path;
   bool should_override{};
};

[[nodiscard]] bool import_level(const LevelImportProps& props);

}// namespace triglav::tool::cli
