#include "String.hpp"

#include <cassert>
#include <utility>

namespace triglav {

namespace {

MemorySize capacity_to_fit_size(const MemorySize size)
{
   auto cap = g_smallStringCapacity;
   while (cap < size) {
      cap <<= 1;
   }
   return cap;
}

}// namespace

StringView::Iterator::Iterator(const char* beg, const char* end, const bool isEnd) :
    m_iterator(beg),
    m_end(end),
    m_isEnd(isEnd)
{
}

StringView::Iterator& StringView::Iterator::operator++()
{
   if (m_iterator == m_end) {
      m_isEnd = true;
      return *this;
   }

   m_rune = decode_rune_from_buffer(m_iterator, m_end);
   return *this;
}

Rune StringView::Iterator::operator*() const
{
   return m_rune;
}

bool StringView::Iterator::operator!=(const StringView::Iterator& other) const
{
   if (m_isEnd == other.m_isEnd)
      return false;

   return m_iterator != other.m_iterator;
}

// ***** StringView *****

bool StringView::operator==(const char* other) const
{
   return *this == StringView{other, std::strlen(other)};
}

bool StringView::operator==(const StringView other) const
{
   if (m_size != other.size()) {
      return false;
   }

   return std::memcmp(this->data(), other.data(), m_size) == 0;
}

const char* StringView::data() const
{
   return m_data;
}

MemorySize StringView::size() const
{
   return m_size;
}

MemorySize StringView::rune_count() const
{
   return calculate_rune_count(m_data, m_size);
}

StringView::Iterator StringView::begin() const
{
   Iterator it{m_data, m_data + m_size, false};
   ++it;
   return it;
}

StringView::Iterator StringView::end() const
{
   return {m_data, m_data + m_size, true};
}

std::string_view StringView::to_std() const
{
   return std::string_view{m_data, m_size};
}

String::String() = default;

String::String(const char* string) :
    String(string, std::strlen(string))
{
}

String::String(const StringView string) :
    String(string.data(), string.size())
{
}

String::String(const char* string, const MemorySize data_size)
{
   this->ensure_capacity_for_size(data_size);
   std::memcpy(this->data(), string, data_size);
}

String::~String()
{
   this->deallocate();
}

String::String(const String& other)
{
   this->ensure_capacity_for_size(other.m_size);
   std::memcpy(this->data(), other.data(), m_size);
}

String& String::operator=(const String& other)
{
   this->ensure_capacity_for_size(other.m_size);
   std::memcpy(this->data(), other.data(), other.m_size);
   return *this;
}

String::String(String&& other) noexcept :
    m_size(std::exchange(other.m_size, 0))
{
   if (m_size <= g_smallStringCapacity) {
      std::memcpy(m_smallPayload.data(), other.m_smallPayload.data(), m_size);
   } else {
      m_largePayload = other.m_largePayload;
   }
}

String& String::operator=(String&& other) noexcept
{
   this->deallocate();

   m_size = std::exchange(other.m_size, 0);
   if (m_size <= g_smallStringCapacity) {
      std::memcpy(m_smallPayload.data(), other.m_smallPayload.data(), m_size);
   } else {
      m_largePayload = other.m_largePayload;
   }

   return *this;
}

bool String::operator==(const String& other) const
{
   return *this == other.view();
}

bool String::operator==(const char* other) const
{
   return *this == StringView(other, std::strlen(other));
}

bool String::operator==(const StringView other) const
{
   if (m_size != other.size()) {
      return false;
   }

   return std::memcmp(this->data(), other.data(), m_size) == 0;
}

const char* String::data() const
{
   if (m_size <= g_smallStringCapacity) {
      return m_smallPayload.data();
   }
   return m_largePayload.data;
}

char* String::data()
{
   if (m_size <= g_smallStringCapacity) {
      return m_smallPayload.data();
   }
   return m_largePayload.data;
}

MemorySize String::size() const
{
   return m_size;
}

MemorySize String::capacity() const
{
   if (m_size <= g_smallStringCapacity) {
      return g_smallStringCapacity;
   }
   return m_largePayload.capacity;
}

MemorySize String::rune_count() const
{
   return calculate_rune_count(this->data(), m_size);
}

StringView String::view() const
{
   return {data(), m_size};
}

bool String::is_empty() const
{
   return m_size == 0;
}

StringView String::subview(i32 initialRune, i32 lastRune) const
{
   const auto runeCount = static_cast<i32>(this->rune_count());
   while (initialRune < 0) {
      initialRune += runeCount;
   }
   while (lastRune < 0) {
      lastRune += runeCount;
   }

   assert(initialRune < runeCount);
   assert(lastRune <= runeCount);
   assert(initialRune <= lastRune);

   auto startPtr = this->data();
   skip_runes(startPtr, this->data() + m_size, initialRune);

   auto endPtr = startPtr;
   skip_runes(endPtr, this->data() + m_size, lastRune - initialRune);

   return {startPtr, static_cast<MemorySize>(endPtr - startPtr)};
}

void String::append(const StringView other)
{
   const auto oldSize = m_size;
   this->grow_to(m_size + other.size());
   std::memcpy(this->data() + oldSize, other.data(), other.size());
}

void String::append_rune(const Rune rune)
{
   const auto count = rune_to_byte_count(rune);
   const auto oldSize = m_size;
   this->grow_to(m_size + count);
   encode_rune_to_buffer(rune, this->data() + oldSize, count);
}

void String::insert_rune_at(const u32 position, const Rune rune)
{
   const auto count = rune_to_byte_count(rune);
   const auto oldSize = m_size;
   this->grow_to(m_size + count);

   auto* ptr = this->data();
   skip_runes(ptr, ptr + oldSize, position);

   const auto offset = ptr - this->data();
   std::memmove(ptr + count, ptr, oldSize - offset);

   encode_rune_to_buffer(rune, ptr, count);
}

void String::remove_rune_at(const u32 position)
{
   auto* runeStart = this->data();
   skip_runes(runeStart, runeStart + m_size, position);

   auto* runeEnd = runeStart;
   skip_runes(runeEnd, this->data() + m_size, 1);

   const auto runeSize = runeEnd - runeStart;
   const auto remainingSize = (this->data() + m_size) - runeEnd;
   std::memmove(runeStart, runeEnd, remainingSize);

   const auto oldSize = m_size;
   const auto newSize = m_size - runeSize;

   if (oldSize > g_smallStringCapacity && newSize <= g_smallStringCapacity) {
      const auto* ptr = this->data();
      std::memcpy(m_smallPayload.data(), ptr, newSize);
      delete[] ptr;
   }

   m_size = newSize;
}

void String::shrink_by(const MemorySize runeCount)
{
   if (m_size == 0 || runeCount == 0)
      return;

   auto newSize = m_size;
   for (MemorySize i = 0; i < runeCount; ++i) {
      do {
         --newSize;
         if (newSize == 0)
            break;
      } while (!is_rune_initial_byte(this->data()[newSize]));

      if (newSize == 0)
         break;
   }

   this->resize(newSize);
}

String::iterator String::begin() const
{
   iterator it{data(), data() + m_size, false};
   ++it;
   return it;
}

String::iterator String::end() const
{
   return {data(), data() + m_size, true};
}

std::string String::to_std() const
{
   return {data(), data() + m_size};
}

void String::grow_to(const MemorySize newSize)
{
   if (newSize <= this->capacity()) {
      m_size = newSize;
      return;
   }

   const auto newCap = capacity_to_fit_size(newSize);
   auto* newData = new char[newCap];
   std::memcpy(newData, this->data(), m_size);

   this->deallocate();
   m_largePayload.capacity = newCap;
   m_largePayload.data = newData;
   m_size = newSize;
}

void String::resize(const MemorySize newSize)
{
   if (newSize == m_size)
      return;
   if (newSize < m_size) {
      if (newSize <= g_smallStringCapacity && m_size > g_smallStringCapacity) {
         const auto* data = m_largePayload.data;
         std::memcpy(m_smallPayload.data(), data, newSize);
         delete[] data;
      }
      m_size = newSize;
   } else {
      const auto oldSize = m_size;
      this->grow_to(newSize);
      std::memset(this->data() + oldSize, ' ', m_size - oldSize);
   }
}

void String::ensure_capacity_for_size(const MemorySize size)
{
   if (size <= this->capacity()) {
      if (m_size > g_smallStringCapacity && size <= g_smallStringCapacity) {
         delete[] m_largePayload.data;
      }
      m_size = size;
      return;
   }
   this->deallocate();

   const auto cap = capacity_to_fit_size(size);
   m_largePayload.capacity = cap;
   m_largePayload.data = new char[cap];
   m_size = size;
}

void String::deallocate()
{
   if (m_size <= g_smallStringCapacity) {
      return;
   }

   delete[] m_largePayload.data;
   m_size = 0;
}

RuneInserterIterator::RuneInserterIterator(String& stringInstance) :
    m_stringInstance(stringInstance)
{
}

RuneInserterIterator& RuneInserterIterator::operator=(const RuneInserterIterator& other)
{
   assert(&m_stringInstance == &other.m_stringInstance);
   m_currentRune = other.m_currentRune;
   return *this;
}

RuneInserterIterator& RuneInserterIterator::operator++()
{
   m_stringInstance.append_rune(m_currentRune);
   return *this;
}

RuneInserterIterator& RuneInserterIterator::operator++(int)
{
   m_stringInstance.append_rune(m_currentRune);
   return *this;
}

RuneInserterIterator& RuneInserterIterator::operator=(const Rune rune)
{
   m_stringInstance.append_rune(rune);
   return *this;
}

Rune& RuneInserterIterator::operator*()
{
   return m_currentRune;
}

RuneInserterIterator rune_inserter(String& stringInstance)
{
   return RuneInserterIterator{stringInstance};
}

CharInserterIterator::CharInserterIterator(String& stringInstance) :
    m_stringInstance(stringInstance)
{
}

CharInserterIterator& CharInserterIterator::operator=(const CharInserterIterator& other)
{
   assert(&m_stringInstance == &other.m_stringInstance);
   m_currentChar = other.m_currentChar;
   return *this;
}

CharInserterIterator& CharInserterIterator::operator++()
{
   if ((m_currentChar & 0x80) == 0) {
      m_stringInstance.append_rune(static_cast<Rune>(m_currentChar));
   } else if ((m_currentChar & 0xc0) == 0xc0) {
      m_charBuffer[0] = m_currentChar;
      m_targetOffset = byte_count_from_initial_byte(m_currentChar);
      m_offset = 1;
   } else {
      m_charBuffer[m_offset] = m_currentChar;
      if (m_offset == m_targetOffset) {
         const auto* ptr = m_charBuffer.data();
         const Rune rune = decode_rune_from_buffer(ptr, ptr + m_charBuffer.size());
         m_stringInstance.append_rune(rune);
      } else {
         ++m_offset;
      }
   }

   return *this;
}

CharInserterIterator& CharInserterIterator::operator++(int)
{
   return this->operator++();
}

CharInserterIterator& CharInserterIterator::operator=(const char ch)
{
   m_currentChar = ch;
   return *this;
}

char& CharInserterIterator::operator*()
{
   return m_currentChar;
}

CharInserterIterator char_inserter(String& stringInstance)
{
   return CharInserterIterator{stringInstance};
}

}// namespace triglav
