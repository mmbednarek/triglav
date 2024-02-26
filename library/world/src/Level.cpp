#include "Level.h"

namespace triglav::world {

void Level::add_node(const NameID id, LevelNode &&node)
{
   m_nodes.emplace(id, std::move(node));
}

LevelNode &Level::at(const NameID id)
{
   return m_nodes.at(id);
}

LevelNode &Level::root()
{
   using namespace name_literals;
   return this->at("root"_name_id);
}

}// namespace triglav::world