#pragma once

#include "triglav/io/Path.h"

#include <optional>

namespace triglav::resource {

class PathManager
{
 public:
   [[nodiscard]] io::Path content_path();
   [[nodiscard]] io::Path build_path();


   [[nodiscard]] static PathManager& the();

 private:
   std::optional<io::Path> m_cachedContentPath;
   std::optional<io::Path> m_cachedBuildPath;
};

}// namespace triglav::resource
