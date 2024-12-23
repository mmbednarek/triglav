#pragma once

#include "Typeface.hpp"

#include <iterator>
#include <string_view>

namespace triglav::font {

class Utf8StringView
{
 public:
   class Iterator
   {
    public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = Rune;
      using pointer = Rune*;
      using reference = Rune&;

      Iterator(std::string_view::const_iterator beg, std::string_view::const_iterator end, bool isEnd);

      Iterator& operator++();
      [[nodiscard]] Rune operator*() const;
      [[nodiscard]] bool operator!=(const Iterator& other) const;

    private:
      Rune m_rune{};
      std::string_view::const_iterator m_iterator;
      std::string_view::const_iterator m_end;
      bool m_isEnd;
   };

   explicit Utf8StringView(std::string_view string);

   [[nodiscard]] Iterator begin() const;
   [[nodiscard]] Iterator end() const;

 private:
   std::string_view m_string;
};

}// namespace triglav::font