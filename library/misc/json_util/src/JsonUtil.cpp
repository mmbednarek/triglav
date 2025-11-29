#include "JsonUtil.hpp"

#include "triglav/io/File.hpp"

namespace triglav::json_util {

RapidJsonInputStream::RapidJsonInputStream(io::IReader& reader) :
    m_reader(reader)
{
}
RapidJsonInputStream::Ch RapidJsonInputStream::Peek() const
{
   if (!m_reader.has_next())
      return 0;

   return m_reader.peek();
}
RapidJsonInputStream::Ch RapidJsonInputStream::Take()
{
   if (!m_reader.has_next())
      return 0;

   m_total_bytes_read += 1;
   return m_reader.next();
}
size_t RapidJsonInputStream::Tell() const
{
   return m_total_bytes_read;
}

RapidJsonInputStream::Ch* RapidJsonInputStream::PutBegin()
{
   assert(false);
   return nullptr;
}

void RapidJsonInputStream::Put(Ch)
{
   assert(false);
}

void RapidJsonInputStream::Flush() {}

size_t RapidJsonInputStream::PutEnd(Ch*)
{
   assert(false);
   return 0;
}

std::optional<rapidjson::Document> create_document_from_file(const io::Path& path)
{
   auto file = io::open_file(path, io::FileMode::Read);
   if (!file.has_value())
      return std::nullopt;

   RapidJsonInputStream stream(**file);

   rapidjson::Document doc;
   doc.ParseStream(stream);

   return doc;
}

}// namespace triglav::json_util
