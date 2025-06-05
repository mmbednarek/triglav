#pragma once

#include "CompTimeString.hpp"
#include "Utf8.hpp"

#include <array>
#include <cstring>

namespace triglav {

constexpr MemorySize g_smallStringCapacity = 16;

class StringView
{
 public:
   class Iterator
   {
    public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = Rune;
      using pointer = Rune*;
      using reference = Rune&;

      Iterator(const char* beg, const char* end, bool isEnd);

      Iterator& operator++();
      [[nodiscard]] Rune operator*() const;
      [[nodiscard]] bool operator!=(const Iterator& other) const;

    private:
      Rune m_rune{};
      const char* m_iterator;
      const char* m_end;
      bool m_isEnd;
   };
   using iterator = Iterator;

   explicit constexpr StringView(const char* string) :
       m_data(string),
       m_size(std::strlen(string))
   {
   }

   constexpr StringView(const char* string, const MemorySize size) :
       m_data(string),
       m_size(size)
   {
   }

   bool operator==(const char* other) const;
   bool operator==(StringView other) const;

   [[nodiscard]] const char* data() const;
   [[nodiscard]] MemorySize size() const;
   [[nodiscard]] MemorySize rune_count() const;

   [[nodiscard]] Iterator begin() const;
   [[nodiscard]] Iterator end() const;

 private:
   const char* m_data;
   MemorySize m_size;
};

class String
{
 public:
   using iterator = StringView::Iterator;

   String();
   String(const char* string);
   String(StringView string);
   String(const char* string, MemorySize data_size);
   ~String();

   String(const String& other);
   String& operator=(const String& other);
   String(String&& other) noexcept;
   String& operator=(String&& other) noexcept;

   bool operator==(const String& other) const;
   bool operator==(const char* other) const;
   bool operator==(StringView other) const;

   [[nodiscard]] const char* data() const;
   [[nodiscard]] char* data();
   [[nodiscard]] MemorySize size() const;
   [[nodiscard]] MemorySize capacity() const;
   [[nodiscard]] MemorySize rune_count() const;
   [[nodiscard]] StringView view() const;
   [[nodiscard]] bool is_empty() const;

   void append(StringView other);
   void append_rune(Rune rune);
   void shrink_by(MemorySize runeCount);

   [[nodiscard]] iterator begin() const;
   [[nodiscard]] iterator end() const;

 private:
   void grow_to(MemorySize newSize);
   void resize(MemorySize newSize);
   void ensure_capacity_for_size(MemorySize size);
   void deallocate();

   struct LargeStringPayload
   {
      char* data;
      MemorySize capacity;
   };

   MemorySize m_size{};
   union
   {
      std::array<char, g_smallStringCapacity> m_smallPayload{};
      LargeStringPayload m_largePayload;
   };
};

class RuneInserterIterator
{
 public:
   using iterator_category = std::output_iterator_tag;
   using value_type = Rune;
   using pointer = Rune*;
   using reference = Rune&;
   using difference_type = ptrdiff_t;

   explicit RuneInserterIterator(String& stringInstance);
   RuneInserterIterator(const RuneInserterIterator& other) = default;
   RuneInserterIterator& operator=(const RuneInserterIterator& other);

   RuneInserterIterator& operator++();
   RuneInserterIterator& operator++(int);
   RuneInserterIterator& operator=(Rune rune);
   [[nodiscard]] Rune& operator*();

 private:
   String& m_stringInstance;
   Rune m_currentRune{};
};

RuneInserterIterator rune_inserter(String& stringInstance);

class CharInserterIterator
{
 public:
   using iterator_category = std::output_iterator_tag;
   using value_type = char;
   using pointer = char*;
   using reference = char&;
   using difference_type = ptrdiff_t;

   explicit CharInserterIterator(String& stringInstance);
   CharInserterIterator(const CharInserterIterator& other) = default;
   CharInserterIterator& operator=(const CharInserterIterator& other);

   CharInserterIterator& operator++();
   CharInserterIterator& operator++(int);
   CharInserterIterator& operator=(char ch);
   [[nodiscard]] char& operator*();

 private:
   String& m_stringInstance;
   char m_currentChar{};
   MemorySize m_offset{};
   MemorySize m_targetOffset{};
   std::array<char, 4> m_charBuffer{};
};

CharInserterIterator char_inserter(String& stringInstance);

namespace string_literals {

template<CompTimeString TValue>
constexpr Rune operator""_rune()
{
   const char* buff = TValue.m_data.data();
   return decode_rune_from_buffer(buff, buff + TValue.m_data.size());
}

template<CompTimeString TValue>
constexpr StringView operator""_strv()
{
   return StringView{TValue.m_data.data(), TValue.m_data.size() - 1};
}

}// namespace string_literals

}// namespace triglav