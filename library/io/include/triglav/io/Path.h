#pragma once

#include <string>
#include <string_view>

#include "Result.h"

namespace triglav::io {

class Path {
 public:
   explicit Path(std::string_view path);

   [[nodiscard]] Path sub(std::string_view value) const;
   [[nodiscard]] Path parent() const;
   [[nodiscard]] bool exists() const;
   [[nodiscard]] const std::string& string() const;
 private:
   std::string m_path;
};

[[nodiscard]] Result<Path> working_path();
[[nodiscard]] Result<std::string> full_path(std::string_view path);
[[nodiscard]] bool is_existing_path(const Path& path);

namespace path_literals {

inline Path operator""_path(const char *value, const std::size_t count)
{
   return Path{std::string{value, count}};
}

}

}
