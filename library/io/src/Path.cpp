#include "Path.h"

#include <algorithm>
#include <fmt/format.h>
#include <ranges>
#include <utility>
#include <cstdlib>

namespace triglav::io {

Path::Path(std::string &&path) :
   m_path(std::move(path))
{
   char result[PATH_MAX];
   ::realpath(m_path.c_str(), result);
   m_path = result;
}

const std::string &Path::string() const
{
   return m_path;
}

Path Path::sub(std::string_view value) const
{
   return Path{fmt::format("{}/{}", m_path, value)};
}

Path Path::parent() const
{
   auto it = std::ranges::find_last(m_path, '/');
   return Path{std::string{m_path.begin(), it.begin()}};
}

}
