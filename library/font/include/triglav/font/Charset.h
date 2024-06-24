#pragma once

#include "Typeface.h"

#include <vector>
#include <string_view>

namespace triglav::font {

struct RuneRange
{
   Rune from;
   Rune to;
};

class Charset
{
 public:
   static const Charset Ascii;
   static const Charset European;

   class Iterator
   {
    public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = Rune;
      using pointer = Rune*;
      using reference = Rune&;

      Iterator(const Charset* charset, std::vector<RuneRange>::const_iterator rangeIt, Rune rune);

      [[nodiscard]] Iterator& operator++();
      [[nodiscard]] Rune operator*() const;
      [[nodiscard]] bool operator!=(const Iterator& other) const;

    private:
      const Charset* m_charset;
      std::vector<RuneRange>::const_iterator m_rangeIt{};
      Rune m_rune{};
   };

   Charset& add_range(Rune from, Rune to);
   [[nodiscard]] std::vector<u32> encode_string(std::string_view str) const;
   [[nodiscard]] u32 count() const;

   [[nodiscard]] Iterator begin() const;
   [[nodiscard]] Iterator end() const;

 private:
   std::vector<RuneRange> m_ranges;
};

}// namespace triglav::font