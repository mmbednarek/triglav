#include "Reader.h"
#include "Parser.h"

namespace object_reader {

Index parse_index(const std::string &index)
{
   const auto it1       = index.find('/');
   const auto vertex_id = std::stoi(index.substr(0, it1));
   const auto it2       = index.find('/', it1 + 1);
   const auto uv_id     = std::stoi(index.substr(it1 + 1, it2 - it1 - 1));
   const auto normal_id = std::stoi(index.substr(it2 + 1));
   return Index(vertex_id, uv_id, normal_id);
}

Object read_object(std::istream &stream)
{
   Object result{};
   Parser parser(stream);
   parser.parse();
   auto &commands = parser.commands();

   for (const auto command : commands) {
      if (command.name == "v") {
         assert(command.arguments.size() == 3);
         result.vertices.emplace_back(std::stof(command.arguments[0]), std::stof(command.arguments[1]),
                                      std::stof(command.arguments[2]));
         continue;
      }
      if (command.name == "vn") {
         assert(command.arguments.size() == 3);
         result.normals.emplace_back(std::stof(command.arguments[0]), std::stof(command.arguments[1]),
                                     std::stof(command.arguments[2]));
         continue;
      }
      if (command.name == "vt") {
         assert(command.arguments.size() == 2);
         result.uvs.emplace_back(std::stof(command.arguments[0]), std::stof(command.arguments[1]));
         continue;
      }
      if (command.name == "f") {
         assert(command.arguments.size() == 3);
         result.faces.emplace_back(std::array<Index, 3>{parse_index(command.arguments[0]),
                                                     parse_index(command.arguments[1]),
                                                     parse_index(command.arguments[2])});
         continue;
      }
   }

   return result;
}

}// namespace object_reader
