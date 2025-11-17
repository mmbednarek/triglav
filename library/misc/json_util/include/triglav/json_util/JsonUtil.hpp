#pragma once

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wnontrivial-memcall"
#elifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996 5054)
#endif
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#ifdef __clang__
#pragma clang diagnostic pop
#elifdef __GNUC__
#pragma GCC diagnostic pop
#elifdef _MSC_VER
#pragma warning(pop)
#endif
#include <optional>

#include "triglav/io/BufferedReader.hpp"
#include "triglav/io/Path.hpp"
#include "triglav/io/Stream.hpp"

namespace triglav::json_util {

class RapidJsonInputStream
{
 public:
   using Ch = char;

   explicit RapidJsonInputStream(io::IReader& reader);

   Ch Peek() const;
   Ch Take();
   size_t Tell() const;
   Ch* PutBegin();
   void Put(Ch /*c*/);
   void Flush();
   size_t PutEnd(Ch* /*begin*/);

 private:
   mutable io::BufferedReader m_reader;
   size_t m_total_bytes_read = 0;
};

std::optional<rapidjson::Document> create_document_from_file(const io::Path& path);

}// namespace triglav::json_util
