#pragma once

#include "CompTimeString.hpp"
#include "Utf8.hpp"

#include <array>
#include <cstring>
#include <format>
#include <string>

namespace triglav {

constexpr MemorySize g_small_string_capacity = 16;

class StringView
{
 public:
   class Iterator
   {
    public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = Rune;
      using difference_type = ptrdiff_t;
      using pointer = Rune*;
      using reference = Rune&;

      Iterator(const char* beg, const char* end, bool is_end);

      Iterator& operator++();
      [[nodiscard]] Rune operator*() const;
      [[nodiscard]] bool operator!=(const Iterator& other) const;

    private:
      Rune m_rune{};
      const char* m_iterator;
      const char* m_end;
      bool m_is_end;
   };
   using iterator = Iterator;

   StringView() :
       m_data(nullptr),
       m_size(0)
   {
   }

   StringView(const char* string) :
       m_data(string),
       m_size(std::strlen(string))
   {
   }

   StringView(const std::string& str) :
       m_data(str.data()),
       m_size(str.size())
   {
   }

   constexpr explicit StringView(const std::string_view std_str) :
       m_data(std_str.data()),
       m_size(std_str.size())
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
   [[nodiscard]] bool is_empty() const;

   [[nodiscard]] Iterator begin() const;
   [[nodiscard]] Iterator end() const;

   [[nodiscard]] std::string_view to_std() const;
   [[nodiscard]] bool starts_with(StringView prefix) const;

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
   String(Rune single_rune, MemorySize num_runes);
   ~String();

   String(const String& other);
   String& operator=(const String& other);
   String(String&& other) noexcept;
   String& operator=(String&& other) noexcept;

   bool operator==(const String& other) const;
   bool operator==(const char* other) const;
   bool operator==(StringView other) const;
   [[nodiscard]] bool operator<(const String& other) const;

   [[nodiscard]] const char* data() const;
   [[nodiscard]] char* data();
   [[nodiscard]] MemorySize size() const;
   [[nodiscard]] MemorySize capacity() const;
   [[nodiscard]] MemorySize rune_count() const;
   [[nodiscard]] StringView view() const;
   [[nodiscard]] bool is_empty() const;

   [[nodiscard]] StringView subview(i32 initial_rune, i32 last_rune) const;

   void append(StringView other);
   void append_rune(Rune rune);
   void shrink_by(MemorySize rune_count);
   void insert_rune_at(u32 position, Rune rune);
   void remove_rune_at(u32 position);
   void remove_range(u32 first_rune, u32 last_rune);

   [[nodiscard]] iterator begin() const;
   [[nodiscard]] iterator end() const;

   [[nodiscard]] std::string to_std() const;
   [[nodiscard]] std::string_view to_std_view() const;

 private:
   void grow_to(MemorySize new_size);
   void resize(MemorySize new_size);
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
      std::array<char, g_small_string_capacity> m_small_payload{};
      LargeStringPayload m_large_payload;
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

   explicit RuneInserterIterator(String& string_instance);
   RuneInserterIterator(const RuneInserterIterator& other) = default;
   RuneInserterIterator& operator=(const RuneInserterIterator& other);

   RuneInserterIterator& operator++();
   RuneInserterIterator& operator++(int);
   RuneInserterIterator& operator=(Rune rune);
   [[nodiscard]] Rune& operator*();

 private:
   String& m_string_instance;
   Rune m_current_rune{};
};

RuneInserterIterator rune_inserter(String& string_instance);

class CharInserterIterator
{
 public:
   using iterator_category = std::output_iterator_tag;
   using value_type = char;
   using pointer = char*;
   using reference = char&;
   using difference_type = ptrdiff_t;

   explicit CharInserterIterator(String& string_instance);
   CharInserterIterator(const CharInserterIterator& other) = default;
   CharInserterIterator& operator=(const CharInserterIterator& other);

   CharInserterIterator& operator++();
   CharInserterIterator& operator++(int);
   CharInserterIterator& operator=(char ch);
   [[nodiscard]] char& operator*();

 private:
   String& m_string_instance;
   char m_current_char{};
   MemorySize m_offset{};
   MemorySize m_target_offset{};
   std::array<char, 4> m_char_buffer{};
};

CharInserterIterator char_inserter(String& string_instance);

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

template<CompTimeString TValue>
constexpr String operator""_str()
{
   return String{TValue.m_data.data(), TValue.m_data.size() - 1};
}

}// namespace string_literals

}// namespace triglav

template<>
struct std::formatter<triglav::String>
{
   constexpr auto parse(std::format_parse_context& ctx)
   {
      return ctx.begin();
   }

   auto format(const triglav::String& obj, std::format_context& ctx) const
   {
      return std::copy_n(obj.data(), obj.size(), ctx.out());
   }
};

template<>
struct std::formatter<triglav::StringView>
{
   constexpr auto parse(std::format_parse_context& ctx)
   {
      return ctx.begin();
   }

   auto format(const triglav::StringView& obj, std::format_context& ctx) const
   {
      return std::copy_n(obj.data(), obj.size(), ctx.out());
   }
};
