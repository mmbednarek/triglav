#pragma once

#include "triglav/Name.hpp"
#include "triglav/io/Path.hpp"
#include "triglav/threading/SafeAccess.hpp"

#include <vector>

namespace triglav::project {

struct PathMapping
{
   std::string input_path_prefix;
   io::Path output_path_prefix;
};

class PathManager
{
 public:
   PathManager();

   [[nodiscard]] io::Path translate_path(ResourceName rc_name) const;
   [[nodiscard]] std::pair<io::Path, ResourceName> import_path(ResourceType rc_type, std::string_view path) const;

   [[nodiscard]] static PathManager& the();

 private:
   threading::SafeReadWriteAccess<std::vector<PathMapping>> m_path_mappings;
};

}// namespace triglav::project
