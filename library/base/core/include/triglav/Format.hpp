#pragma once

#include "String.hpp"

#include <format>
#include <vector>

namespace triglav {

template<typename... T>
[[nodiscard]] String format(std::format_string<T...> fmt, T&&... args)
{
   String str;
   std::format_to(char_inserter(str), fmt, std::forward<T>(args)...);
   return str;
}

}// namespace triglav