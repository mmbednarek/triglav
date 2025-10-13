#pragma once

#include "LevelNode.hpp"

#include <map>

namespace triglav::io {
class Path;
}

namespace triglav::world {

class Level
{
 public:
   void add_node(Name id, LevelNode&& node);

   LevelNode& at(Name id);
   LevelNode& root();

   void serialize_yaml(c4::yml::NodeRef& node) const;
   [[nodiscard]] bool save_to_file(const io::Path& path) const;

 private:
   std::map<Name, LevelNode> m_nodes;
};

}// namespace triglav::world