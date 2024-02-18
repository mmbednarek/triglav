#include "File.h"

#include <fstream>

namespace triglav::io {

std::vector<char> read_whole_file(const std::string_view name)
{
   std::ifstream file(std::string{name}, std::ios::ate | std::ios::binary);
   if (not file.is_open()) {
      return {};
   }

   file.seekg(0, std::ios::end);
   const auto fileSize = file.tellg();
   file.seekg(0, std::ios::beg);

   std::vector<char> result{};
   result.resize(fileSize);

   file.read(result.data(), fileSize);
   return result;
}

}// namespace triglav::io
