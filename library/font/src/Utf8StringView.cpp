#include "Utf8StringView.h"

#include <cassert>

namespace triglav::font {

// ***** Utf8StringView::Iterator *****

Utf8StringView::Iterator::Iterator(std::string_view::const_iterator beg, std::string_view::const_iterator end, bool isEnd) :
    m_iterator(beg),
    m_end(end),
    m_isEnd(isEnd)
{
}

Utf8StringView::Iterator& Utf8StringView::Iterator::operator++()
{
   if (m_iterator == m_end) {
      m_isEnd = true;
      return *this;
   }

   const auto ch = static_cast<u8>(*m_iterator);

   if ((ch & 0x80) == 0) {
      m_rune = ch;
      ++m_iterator;
      return *this;
   }

   u32 byteCount = 1;
   auto byteIndicator = ch;
   byteIndicator <<= 2;
   while ((byteIndicator & 0x80) != 0) {
      ++byteCount;
      byteIndicator <<= 1;
   }

   const u32 bitsInByte = 6 - byteCount;
   const u8 mask = (1 << bitsInByte) - 1;

   m_rune = ch & mask;

   for (u32 i = 0; i < byteCount; ++i) {
      ++m_iterator;
      assert(m_iterator != m_end);
      assert((*m_iterator & 0xc0) == 0x80);

      m_rune = (m_rune << 6) | (*m_iterator & 0x3f);
   }

   ++m_iterator;
   return *this;
}

Rune Utf8StringView::Iterator::operator*() const
{
   return m_rune;
}

bool Utf8StringView::Iterator::operator!=(const Utf8StringView::Iterator& other) const
{
   return m_isEnd != other.m_isEnd || m_iterator != other.m_iterator;
}

// ***** Utf8StringView *****

Utf8StringView::Utf8StringView(std::string_view string) :
   m_string(string)
{
}

Utf8StringView::Iterator Utf8StringView::begin() const
{
   Iterator it{m_string.begin(), m_string.end(), false};
   ++it;
   return it;
}

Utf8StringView::Iterator Utf8StringView::end() const
{
   return {m_string.end(), m_string.end(), true};
}

}// namespace triglav::font
