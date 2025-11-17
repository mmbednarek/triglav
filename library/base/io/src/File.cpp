#include "File.hpp"

#include <fstream>

namespace triglav::io {

std::vector<char> read_whole_file(const Path& path)
{
   auto file_res = open_file(path, FileOpenMode::Read);
   if (not file_res.has_value()) {
      return {};
   }

   auto& file = **file_res;
   const auto file_size = file.file_size();
   if (not file_size.has_value()) {
      return {};
   }

   std::vector<char> result{};
   result.resize(*file_size);

   const auto file_read_res = file.read(std::span{reinterpret_cast<u8*>(result.data()), result.size()});
   if (not file_read_res.has_value()) {
      return {};
   }

   return result;
}

}// namespace triglav::io
