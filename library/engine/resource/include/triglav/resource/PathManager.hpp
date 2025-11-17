#pragma once

#include "triglav/io/Path.hpp"
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
   threading::SafeReadWriteAccess<std::optional<io::Path>> m_cached_content_path;
   threading::SafeReadWriteAccess<std::optional<io::Path>> m_cached_build_path;
};

}// namespace triglav::resource
