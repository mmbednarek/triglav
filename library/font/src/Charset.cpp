#include "Charset.hpp"

namespace triglav::font {

const Charset Charset::Ascii = Charset().add_range(32, 126);
const Charset Charset::European = Charset().add_range(32, 126).add_range(0xc0, 0x17f);

// ***** Charset::Iterator *****

Charset::Iterator::Iterator(const Charset* charset, const std::vector<RuneRange>::const_iterator rangeIt, const Rune rune) :
    m_charset(charset),
    m_rangeIt(rangeIt),
    m_rune(rune)
{
}

Charset::Iterator& Charset::Iterator::operator++()
{
   if (m_rune >= m_rangeIt->to) {
      ++m_rangeIt;
      if (m_rangeIt != m_charset->m_ranges.end()) {
         m_rune = m_rangeIt->from;
      } else {
         m_rune = 0;
      }
      return *this;
   }

   ++m_rune;
   return *this;
}

Rune Charset::Iterator::operator*() const
{
   return m_rune;
}

bool Charset::Iterator::operator!=(const Charset::Iterator& other) const
{
   return m_rangeIt != other.m_rangeIt || m_rune != other.m_rune;
}

// ***** Charset *****

Charset& Charset::add_range(Rune from, Rune to)
{
   m_ranges.emplace_back(from, to);
   return *this;
}

std::vector<u32> Charset::encode_string(const StringView str) const
{
   std::vector<u32> result;
   result.reserve(str.size());
   this->encode_string_to(str, std::back_inserter(result));
   return result;
}

u32 Charset::count() const
{
   u32 count{};
   for (const auto& [from, to] : m_ranges) {
      count += to - from + 1;
   }
   return count;
}

Charset::Iterator Charset::begin() const
{
   if (m_ranges.empty()) {
      return Iterator{this, m_ranges.end(), 0};
   }
   return Iterator{this, m_ranges.begin(), m_ranges.front().from};
}

Charset::Iterator Charset::end() const
{
   return Iterator{this, m_ranges.end(), 0};
}

}// namespace triglav::font
