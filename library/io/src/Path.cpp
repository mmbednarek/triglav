#include "Path.h"

#include <algorithm>
#include <cstdlib>
#include <fmt/format.h>
#include <ranges>
#include <utility>

namespace triglav::io {

Path::Path(const std::string_view path) :
    m_path(path)
{
   auto fullPath = full_path(m_path);
   if (fullPath.has_value()) {
      m_path = fullPath.value();
   }
}

const std::string& Path::string() const
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

bool Path::exists() const
{
   return is_existing_path(*this);
}

}// namespace triglav::io
