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

constexpr void encode_rune_to_buffer(const Rune rune, char* buffer, const MemorySize byteCount)
{
   switch (byteCount) {
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

constexpr u32 byte_count_from_initial_byte(u8 initialByte)
{
   u32 byteCount = 1;
   initialByte <<= 2;
   while ((initialByte & 0x80) != 0) {
      ++byteCount;
      initialByte <<= 1;
   }
   return byteCount;
}

constexpr Rune decode_rune_from_buffer(const char*& buffer, const char* endPtr)
{
   const auto ch = static_cast<u8>(*buffer);
   if ((ch & 0x80) == 0) {
      ++buffer;
      return ch;
   }

   const u32 byteCount = byte_count_from_initial_byte(ch);
   const u32 bitsInByte = 6 - byteCount;
   const u8 mask = (1 << bitsInByte) - 1;

   Rune result = ch & mask;

   for (u32 i = 0; i < byteCount; ++i) {
      ++buffer;
      assert(buffer != endPtr);
      assert((*buffer & 0xc0) == 0x80);

      result = (result << 6) | (*buffer & 0x3f);
   }

   ++buffer;
   return result;
}

}// namespace triglav
