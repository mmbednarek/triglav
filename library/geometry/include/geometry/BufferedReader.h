#pragma once

#include <array>
#include <istream>

namespace geometry {

constexpr auto g_bufferSize = 256;

class BufferedReader
{
public:
   explicit BufferedReader(std::istream& stream);

   [[nodiscard]] bool has_next();
   [[nodiscard]] char next();

private:
   void read_next_chunk();

   std::array<char, g_bufferSize> m_buffer{};
   size_t m_position{0};
   size_t m_bytesRead{g_bufferSize};
   std::istream& m_stream;
   int m_line{1};
   int m_column{1};
};

}