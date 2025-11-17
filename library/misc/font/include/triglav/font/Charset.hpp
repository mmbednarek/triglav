#pragma once

#include "Typeface.hpp"

#include "triglav/String.hpp"

#include <string_view>
#include <vector>

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

      Iterator(const Charset* charset, std::vector<RuneRange>::const_iterator range_it, Rune rune);

      Iterator& operator++();
      [[nodiscard]] Rune operator*() const;
      [[nodiscard]] bool operator!=(const Iterator& other) const;

    private:
      const Charset* m_charset;
      std::vector<RuneRange>::const_iterator m_range_it{};
      Rune m_rune{};
   };

   Charset& add_range(Rune from, Rune to);
   [[nodiscard]] std::vector<u32> encode_string(StringView str) const;
   [[nodiscard]] bool contains(Rune r) const;

   template<typename TIterator>
   u32 encode_string_to(const StringView str, TIterator out_it) const
   {
      u32 count{};

      for (const auto rune : str) {
         u32 index_base = 0;
         for (const auto& [from, to] : m_ranges) {
            if (rune >= from && rune <= to) {
               *out_it = index_base + rune - from;
               ++out_it;
               ++count;
               break;
            }
            index_base += to - from + 1;
         }
      }

      return count;
   }

   [[nodiscard]] u32 count() const;

   [[nodiscard]] Iterator begin() const;
   [[nodiscard]] Iterator end() const;

 private:
   std::vector<RuneRange> m_ranges;
};

}// namespace triglav::font