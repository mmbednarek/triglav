#pragma once

#include "triglav/Name.hpp"
#include "triglav/io/Path.hpp"
#include "triglav/threading/SafeAccess.hpp"

#include <optional>
#include <vector>

namespace triglav::resource {

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

   [[nodiscard]] static PathManager& the();

 private:
   threading::SafeReadWriteAccess<std::vector<PathMapping>> m_path_mappings;
};

}// namespace triglav::resource
