#pragma once

#include "triglav/io/Path.h"
#include "triglav/threading/SafeAccess.hpp"

#include <optional>

namespace triglav::resource {

class PathManager
{
 public:
   [[nodiscard]] io::Path content_path();
   [[nodiscard]] io::Path build_path();


   [[nodiscard]] static PathManager& the();

 private:
   threading::SafeReadWriteAccess<std::optional<io::Path>> m_cachedContentPath;
   threading::SafeReadWriteAccess<std::optional<io::Path>> m_cachedBuildPath;
};

}// namespace triglav::resource
