#pragma once

#include "LevelNode.h"

#include <map>

namespace triglav::world {

class Level {
public:
  void add_node(NameID id, LevelNode&& node);

  LevelNode& at(NameID id);
  LevelNode& root();

private:
  std::map<NameID, LevelNode> m_nodes;
};

}