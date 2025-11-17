#pragma once

#include "Int.hpp"

#include <cassert>

namespace triglav {

using Rune = u32;

constexpr bool is_rune_initial_byte(const char ch)
{
   return (ch & 0xc0) != 0x80;
}

constexpr MemorySize calculate_rune_count(const char* data, const MemorySize size)
{
   MemorySize count{};

   for (MemorySize i = 0; i < size; ++i) {
      if (is_rune_initial_byte(data[i])) {
         ++count;
      }
   }

   return count;
}

[[nodiscard]] constexpr MemorySize rune_to_byte_count(const Rune rune)
{
   if (rune <= 0x7f)
      return 1;
   if (rune <= 0x7ff)
      return 2;
   if (rune <= 0xffff)
      return 3;
   return 4;
}

constexpr void encode_rune_to_buffer(const Rune rune, char* buffer, const MemorySize byte_count)
{
   switch (byte_count) {
   case 0:
      break;
   case 1:
      buffer[0] = static_cast<char>(rune);
      break;
   case 2:
      buffer[0] = static_cast<char>(0xc0 | ((rune >> 6) & 0x1f));
      buffer[1] = static_cast<char>(0x80 | (rune & 0x3f));
      break;
   case 3:
      buffer[0] = static_cast<char>(0xe0 | ((rune >> 12) & 0xf));
      buffer[1] = static_cast<char>(0x80 | ((rune >> 6) & 0x3f));
      buffer[2] = static_cast<char>(0x80 | (rune & 0x3f));
      break;
   case 4:
      buffer[0] = static_cast<char>(0xf0 | ((rune >> 18) & 0x7));
      buffer[1] = static_cast<char>(0x80 | ((rune >> 12) & 0x3f));
      buffer[2] = static_cast<char>(0x80 | ((rune >> 6) & 0x3f));
      buffer[3] = static_cast<char>(0x80 | (rune & 0x3f));
      break;
   default:
      assert(0 && "too many bytes to encode an UTF-8 character");
   }
}

constexpr u32 byte_count_from_initial_byte(u8 initial_byte)
{
   u32 byte_count = 1;
   initial_byte <<= 2;
   while ((initial_byte & 0x80) != 0) {
      ++byte_count;
      initial_byte <<= 1;
   }
   return byte_count;
}

constexpr Rune decode_rune_from_buffer(const char*& buffer, [[maybe_unused]] const char* end_ptr)
{
   const auto ch = static_cast<u8>(*buffer);
   if ((ch & 0x80) == 0) {
      ++buffer;
      return ch;
   }

   const u32 byte_count = byte_count_from_initial_byte(ch);
   const u32 bits_in_byte = 6 - byte_count;
   const u8 mask = (1 << bits_in_byte) - 1;

   Rune result = ch & mask;

   for (u32 i = 0; i < byte_count; ++i) {
      ++buffer;
      assert(buffer != end_ptr);
      assert((*buffer & 0xc0) == 0x80);

      result = (result << 6) | (*buffer & 0x3f);
   }

   ++buffer;
   return result;
}

template<typename T>
constexpr void skip_runes(T& buffer, T end_ptr, u32 rune_count)
{
   while (buffer != end_ptr && (rune_count != 0 || !is_rune_initial_byte(*buffer))) {
      if (is_rune_initial_byte(*buffer)) {
         --rune_count;
      }
      ++buffer;
   }
}

}// namespace triglav
