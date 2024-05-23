#pragma once

#include <string>

#include "Result.h"

namespace triglav::io {

class Path {
 public:
   explicit Path(std::string&& path);

   [[nodiscard]] Path sub(std::string_view value) const;
   [[nodiscard]] Path parent() const;
   [[nodiscard]] const std::string& string() const;
 private:
   std::string m_path;
};

[[nodiscard]] Result<Path> working_path();

namespace path_literals {

inline Path operator""_path(const char *value, const std::size_t count)
{
   return Path{std::string{value, count}};
}

}

}
