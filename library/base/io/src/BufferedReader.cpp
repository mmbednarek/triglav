#include "BufferedReader.hpp"

namespace triglav::io {

BufferedReader::BufferedReader(IReader& stream) :
    m_stream(stream)
{
   this->read_next_chunk();
}

bool BufferedReader::has_next()
{
   if (m_bytes_read == 0)
      return false;

   if (m_position >= m_bytes_read) {
      this->read_next_chunk();
   }

   return m_position < m_bytes_read;
}

u8 BufferedReader::next()
{
   const auto result = this->peek();
   ++m_position;
   return result;
}

u8 BufferedReader::peek()
{
   if (m_position >= m_bytes_read) {
      read_next_chunk();
   }
   if (m_position >= m_bytes_read) {
      return static_cast<u8>(EOF);
   }

   const auto result = m_buffer[m_position];
   if (result == '\n') {
      m_column = 1;
      ++m_line;
   } else {
      ++m_column;
   }

   return result;
}

void BufferedReader::read_next_chunk()
{
   const auto res = m_stream.read(m_buffer);
   if (not res.has_value()) {
      m_bytes_read = 0;
      m_position = 0;
      return;
   }
   m_bytes_read = *res;
   m_position = 0;
}

}// namespace triglav::io