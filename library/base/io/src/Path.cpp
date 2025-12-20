#include "Path.hpp"

#include <algorithm>
#include <cstdlib>
#include <ranges>
#include <utility>

namespace triglav::io {

Path::Path() :
    m_path{}
{
}

Path::Path(const std::string_view path) :
    m_path(path)
{
   auto p = full_path(m_path);
   if (p.has_value()) {
      m_path = p.value();
   }
}

const std::string& Path::string() const
{
   return m_path;
}

std::string Path::basename() const
{
   auto it = std::find(m_path.rbegin(), m_path.rend(), path_seperator());
   return std::string{it.base(), m_path.end()};
}

Path Path::sub(const std::string_view value) const
{
   return Path{sub_directory(this->m_path, value)};
}

Path Path::parent() const
{
   auto it = std::find(m_path.rbegin(), m_path.rend(), path_seperator());
   if (it == m_path.rend())
      return Path{""};
   return Path{std::string{m_path.begin(), it.base() - 1}};
}

bool Path::exists() const
{
   return is_existing_path(*this);
}

}// namespace triglav::io
