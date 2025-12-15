#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace triglav::detail {

constexpr u64 CRC64_ECMA_POLY = 0x42F0E1EBA9EA3693ULL;

constexpr std::array<u64, 256> crc64_generate_table()
{
   std::array<u64, 256> result;

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

constexpr uint64_t compute_crc(const char* data, const uint32_t len, uint64_t crc = 0)
{
   for (uint64_t i = 0; i < len; i++) {
      crc = CRC64_LOOKUP[*data ^ (crc & 0xFF)] ^ (crc >> 8);
      data++;
   }
   return crc;
}

constexpr auto hash_string(const std::string_view value)
{
   return compute_crc(value.data(), static_cast<uint32_t>(value.size()));
}

}// namespace triglav::detail
