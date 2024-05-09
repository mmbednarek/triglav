#pragma once

#include <algorithm>
#include <array>

namespace triglav {

template<typename std::array<char, 0>::size_type CSize>
struct CompTimeString
{
   using size_type = typename std::array<char, 0>::size_type;

   std::array<char, CSize> m_data;

   constexpr CompTimeString(const char (&value)[CSize])
   {
      std::copy_n(value, CSize, m_data.begin());
   }

   [[nodiscard]] constexpr auto begin() const
   {
      return m_data.begin();
   }

   [[nodiscard]] constexpr auto end() const
   {
      return m_data.end();
   }

   [[nodiscard]] constexpr bool operator==(const std::string_view other) const
   {
      if (other.size() != (CSize - 1))
         return false;
      return std::all_of(m_data.begin(), m_data.end(),
                         [at = other.begin()](char c) mutable { return c == *(at++); });
   }

   template<size_type COtherSize>
   [[nodiscard]] constexpr bool operator==(const CompTimeString<COtherSize> &other) const
   {
      if (COtherSize != CSize)
         return false;
      return std::all_of(begin(), end(), [at = other.begin()](char c) mutable { return c == *(at++); });
   }

   [[nodiscard]] constexpr std::string_view to_string_view() const
   {
      return std::string_view{m_data.data(), CSize - 1};
   }
};

}// namespace triglav