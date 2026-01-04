#pragma once

#include "../Int.hpp"

#include <array>
#include <string_view>

namespace triglav::detail {

constexpr u64 CRC64_ECMA_POLY = 0x42F0E1EBA9EA3693ULL;

constexpr std::array<u64, 256> crc64_generate_table()
{
   std::array<u64, 256> result{};

   for (u64 i = 0; i < 256; i++) {
      u64 crc = 0;
      u64 c = i << 56;
      for (u64 j = 0; j < 8; j++) {
         if ((crc ^ c) & 0x8000000000000000ULL)
            crc = (crc << 1) ^ CRC64_ECMA_POLY;
         else
            crc <<= 1;
         c <<= 1;
      }
      result[i] = crc;
   }

   return result;
}

static constexpr auto CRC64_LOOKUP = crc64_generate_table();

constexpr auto hash_string(const std::string_view str)
{
   u64 crc = 0;
   for (const char ch : str) {
      crc = CRC64_LOOKUP[static_cast<u8>(ch) ^ (crc & 0xFF)] ^ (crc >> 8);
   }
   return crc;
}

}// namespace triglav::detail
