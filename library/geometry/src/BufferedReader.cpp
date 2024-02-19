#include "BufferedReader.h"

namespace triglav::geometry {

BufferedReader::BufferedReader(std::istream &stream) :
   m_stream(stream)
{
}

bool BufferedReader::has_next()
{
   if (m_bytesRead == 0)
      return false;

   if (m_position >= m_bytesRead) {
      this->read_next_chunk();
   }

   return m_position < m_bytesRead;
}

char BufferedReader::next()
{
   if (m_position >= m_bytesRead) {
      read_next_chunk();
   }
   if (m_position >= m_bytesRead) {
      return EOF;
   }

   const char result = m_buffer[m_position];
   if (result == '\n') {
      m_column = 1;
      ++m_line;
   } else {
      ++m_column;
   }

   ++m_position;
   return result;
}

void BufferedReader::read_next_chunk()
{
   m_bytesRead = m_stream.readsome(m_buffer.data(), static_cast<std::streamsize>(m_buffer.size()));
   m_position = 0;
}

}// namespace object_reader