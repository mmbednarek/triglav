#pragma once

#include "Stream.hpp"

#include <array>
#include <istream>

namespace triglav::io {

constexpr auto g_bufferSize = 1024;

class BufferedReader
{
 public:
   explicit BufferedReader(IReader& stream);

   [[nodiscard]] bool has_next();
   u8 next();
   [[nodiscard]] u8 peek();

 private:
   void read_next_chunk();

   std::array<u8, g_bufferSize> m_buffer{};
   size_t m_position{0};
   size_t m_bytesRead{0};
   IReader& m_stream;
   int m_line{1};
   int m_column{1};
};

}// namespace triglav::io