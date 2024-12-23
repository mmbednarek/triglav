#include "Level.hpp"

namespace triglav::world {

void Level::add_node(const Name id, LevelNode&& node)
{
   m_nodes.emplace(id, std::move(node));
}

LevelNode& Level::at(const Name id)
{
   return m_nodes.at(id);
}

LevelNode& Level::root()
{
   using namespace name_literals;
   return this->at("root"_name);
}

}// namespace triglav::world