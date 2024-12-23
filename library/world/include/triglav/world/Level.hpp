#pragma once

#include "LevelNode.hpp"

#include <map>

namespace triglav::world {

class Level
{
 public:
   void add_node(Name id, LevelNode&& node);

   LevelNode& at(Name id);
   LevelNode& root();

 private:
   std::map<Name, LevelNode> m_nodes;
};

}// namespace triglav::world