#pragma once

#include "triglav/io/Path.hpp"
#include "triglav/threading/SafeAccess.hpp"

#include <optional>

namespace triglav::resource {

class PathManager
{
 public:
   [[nodiscard]] const io::Path& content_path();
   [[nodiscard]] std::optional<io::Path> engine_content_path();
   [[nodiscard]] std::optional<io::Path> project_content_path();
   [[nodiscard]] const io::Path& build_path();

   [[nodiscard]] static PathManager& the();

 private:
   threading::SafeReadWriteAccess<std::optional<io::Path>> m_cached_content_path;
   threading::SafeReadWriteAccess<std::optional<io::Path>> m_cached_engine_content_path;
   threading::SafeReadWriteAccess<std::optional<io::Path>> m_cached_project_content_path;
   threading::SafeReadWriteAccess<std::optional<io::Path>> m_cached_build_path;
};

}// namespace triglav::resource
