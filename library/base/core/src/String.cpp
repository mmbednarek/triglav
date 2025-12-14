#include "String.hpp"

#include <cassert>
#include <utility>

namespace triglav {

namespace {

MemorySize capacity_to_fit_size(const MemorySize size)
{
   auto cap = g_small_string_capacity;
   while (cap < size) {
      cap <<= 1;
   }
   return cap;
}

}// namespace

StringView::Iterator::Iterator(const char* beg, const char* end, const bool is_end) :
    m_iterator(beg),
    m_end(end),
    m_is_end(is_end)
{
}

StringView::Iterator& StringView::Iterator::operator++()
{
   if (m_iterator == m_end) {
      m_is_end = true;
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
   if (m_is_end == other.m_is_end)
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

bool StringView::is_empty() const
{
   return m_size == 0;
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

String::String(const Rune single_rune, const MemorySize num_runes)
{
   const auto rune_byte_count = rune_to_byte_count(single_rune);
   this->ensure_capacity_for_size(num_runes * rune_byte_count);
   if (rune_byte_count == 1) {
      std::memset(this->data(), static_cast<int>(single_rune), num_runes);
   } else {
      std::array<char, 8> rune_chars{};
      encode_rune_to_buffer(single_rune, rune_chars.data(), rune_byte_count);
      for (MemorySize i = 0; i < num_runes; ++i) {
         std::memcpy(this->data() + i * rune_byte_count, rune_chars.data(), rune_byte_count);
      }
   }
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
   if (m_size <= g_small_string_capacity) {
      std::memcpy(m_small_payload.data(), other.m_small_payload.data(), m_size);
   } else {
      m_large_payload = other.m_large_payload;
   }
}

String& String::operator=(String&& other) noexcept
{
   this->deallocate();

   m_size = std::exchange(other.m_size, 0);
   if (m_size <= g_small_string_capacity) {
      std::memcpy(m_small_payload.data(), other.m_small_payload.data(), m_size);
   } else {
      m_large_payload = other.m_large_payload;
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
   if (m_size <= g_small_string_capacity) {
      return m_small_payload.data();
   }
   return m_large_payload.data;
}

char* String::data()
{
   if (m_size <= g_small_string_capacity) {
      return m_small_payload.data();
   }
   return m_large_payload.data;
}

MemorySize String::size() const
{
   return m_size;
}

MemorySize String::capacity() const
{
   if (m_size <= g_small_string_capacity) {
      return g_small_string_capacity;
   }
   return m_large_payload.capacity;
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

StringView String::subview(i32 initial_rune, i32 last_rune) const
{
   const auto rune_count = static_cast<i32>(this->rune_count());
   while (initial_rune < 0) {
      initial_rune += rune_count;
   }
   while (last_rune < 0) {
      last_rune += rune_count;
   }

   assert(initial_rune < rune_count);
   assert(last_rune <= rune_count);
   assert(initial_rune <= last_rune);

   auto start_ptr = this->data();
   skip_runes(start_ptr, this->data() + m_size, initial_rune);

   auto end_ptr = start_ptr;
   skip_runes(end_ptr, this->data() + m_size, last_rune - initial_rune);

   return {start_ptr, static_cast<MemorySize>(end_ptr - start_ptr)};
}

void String::append(const StringView other)
{
   const auto old_size = m_size;
   this->grow_to(m_size + other.size());
   std::memcpy(this->data() + old_size, other.data(), other.size());
}

void String::append_rune(const Rune rune)
{
   const auto count = rune_to_byte_count(rune);
   const auto old_size = m_size;
   this->grow_to(m_size + count);
   encode_rune_to_buffer(rune, this->data() + old_size, count);
}

void String::insert_rune_at(const u32 position, const Rune rune)
{
   const auto count = rune_to_byte_count(rune);
   const auto old_size = m_size;
   this->grow_to(m_size + count);

   auto* ptr = this->data();
   skip_runes(ptr, ptr + old_size, position);

   const auto offset = ptr - this->data();
   std::memmove(ptr + count, ptr, old_size - offset);

   encode_rune_to_buffer(rune, ptr, count);
}

void String::remove_rune_at(const u32 position)
{
   this->remove_range(position, position + 1);
}

void String::remove_range(const u32 first_rune, const u32 last_rune)
{
   auto* start = this->data();
   skip_runes(start, start + m_size, first_rune);

   auto* end = start;
   skip_runes(end, this->data() + m_size, last_rune - first_rune);

   const auto data_size = end - start;
   const auto remaining_size = (this->data() + m_size) - end;
   std::memmove(start, end, remaining_size);

   const auto old_size = m_size;
   const auto new_size = m_size - data_size;

   if (old_size > g_small_string_capacity && new_size <= g_small_string_capacity) {
      const auto* ptr = this->data();
      std::memcpy(m_small_payload.data(), ptr, new_size);
      delete[] ptr;
   }

   m_size = new_size;
}

void String::shrink_by(const MemorySize rune_count)
{
   if (m_size == 0 || rune_count == 0)
      return;

   auto new_size = m_size;
   for (MemorySize i = 0; i < rune_count; ++i) {
      do {
         --new_size;
         if (new_size == 0)
            break;
      } while (!is_rune_initial_byte(this->data()[new_size]));

      if (new_size == 0)
         break;
   }

   this->resize(new_size);
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

void String::grow_to(const MemorySize new_size)
{
   if (new_size <= this->capacity()) {
      m_size = new_size;
      return;
   }

   const auto new_cap = capacity_to_fit_size(new_size);
   auto* new_data = new char[new_cap];
   std::memcpy(new_data, this->data(), m_size);

   this->deallocate();
   m_large_payload.capacity = new_cap;
   m_large_payload.data = new_data;
   m_size = new_size;
}

void String::resize(const MemorySize new_size)
{
   if (new_size == m_size)
      return;
   if (new_size < m_size) {
      if (new_size <= g_small_string_capacity && m_size > g_small_string_capacity) {
         const auto* data = m_large_payload.data;
         std::memcpy(m_small_payload.data(), data, new_size);
         delete[] data;
      }
      m_size = new_size;
   } else {
      const auto old_size = m_size;
      this->grow_to(new_size);
      std::memset(this->data() + old_size, ' ', m_size - old_size);
   }
}

void String::ensure_capacity_for_size(const MemorySize size)
{
   if (size <= this->capacity()) {
      if (m_size > g_small_string_capacity && size <= g_small_string_capacity) {
         delete[] m_large_payload.data;
      }
      m_size = size;
      return;
   }
   this->deallocate();

   const auto cap = capacity_to_fit_size(size);
   m_large_payload.capacity = cap;
   m_large_payload.data = new char[cap];
   m_size = size;
}

void String::deallocate()
{
   if (m_size <= g_small_string_capacity) {
      return;
   }

   delete[] m_large_payload.data;
   m_size = 0;
}

RuneInserterIterator::RuneInserterIterator(String& string_instance) :
    m_string_instance(string_instance)
{
}

RuneInserterIterator& RuneInserterIterator::operator=(const RuneInserterIterator& other)
{
   assert(&m_string_instance == &other.m_string_instance);
   m_current_rune = other.m_current_rune;
   return *this;
}

RuneInserterIterator& RuneInserterIterator::operator++()
{
   m_string_instance.append_rune(m_current_rune);
   return *this;
}

RuneInserterIterator& RuneInserterIterator::operator++(int)
{
   m_string_instance.append_rune(m_current_rune);
   return *this;
}

RuneInserterIterator& RuneInserterIterator::operator=(const Rune rune)
{
   m_string_instance.append_rune(rune);
   return *this;
}

Rune& RuneInserterIterator::operator*()
{
   return m_current_rune;
}

RuneInserterIterator rune_inserter(String& string_instance)
{
   return RuneInserterIterator{string_instance};
}

CharInserterIterator::CharInserterIterator(String& string_instance) :
    m_string_instance(string_instance)
{
}

CharInserterIterator& CharInserterIterator::operator=(const CharInserterIterator& other)
{
   assert(&m_string_instance == &other.m_string_instance);
   m_current_char = other.m_current_char;
   return *this;
}

CharInserterIterator& CharInserterIterator::operator++()
{
   if ((m_current_char & 0x80) == 0) {
      m_string_instance.append_rune(static_cast<Rune>(m_current_char));
   } else if ((m_current_char & 0xc0) == 0xc0) {
      m_char_buffer[0] = m_current_char;
      m_target_offset = byte_count_from_initial_byte(m_current_char);
      m_offset = 1;
   } else {
      m_char_buffer[m_offset] = m_current_char;
      if (m_offset == m_target_offset) {
         const auto* ptr = m_char_buffer.data();
         const Rune rune = decode_rune_from_buffer(ptr, ptr + m_char_buffer.size());
         m_string_instance.append_rune(rune);
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
   m_current_char = ch;
   return *this;
}

char& CharInserterIterator::operator*()
{
   return m_current_char;
}

CharInserterIterator char_inserter(String& string_instance)
{
   return CharInserterIterator{string_instance};
}

}// namespace triglav
