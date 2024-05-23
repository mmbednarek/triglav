#include "File.h"

#include <fstream>

namespace triglav::io {

std::vector<char> read_whole_file(const Path& path)
{
   auto fileRes = open_file(path, FileOpenMode::Read);
   if (not fileRes.has_value()) {
      return {};
   }

   auto &file          = **fileRes;
   const auto fileSize = file.file_size();
   if (not fileSize.has_value()) {
      return {};
   }

   std::vector<char> result{};
   result.resize(*fileSize);

   const auto fileReadRes = file.read(std::span{reinterpret_cast<u8 *>(result.data()), result.size()});
   if (not fileReadRes.has_value()) {
      return {};
   }

   return result;
}

}// namespace triglav::io
