#include "Level.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/io/File.hpp"
#include "triglav/io/Path.hpp"

#include <c4/substr.hpp>
#include <ryml.hpp>

namespace c4 {
inline c4::substr to_substr(std::string& s) noexcept
{
   return {s.data(), s.size()};
}

inline c4::csubstr to_csubstr(std::string const& s) noexcept
{
   return {s.data(), s.size()};
}
}// namespace c4

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

void Level::serialize_yaml(c4::yml::NodeRef& node) const
{
   auto nodes_yaml = node["nodes"];
   nodes_yaml |= ryml::SEQ;
   for (const auto& level_node : Values(m_nodes)) {
      auto child = nodes_yaml.append_child();
      child |= ryml::MAP;
      level_node.serialize_yaml(child);
   }
}

bool Level::save_to_file(const io::Path& path) const
{
   const auto file = io::open_file(path, io::FileOpenMode::Create);
   if (!file.has_value()) {
      return false;
   }

   ryml::Tree tree;
   ryml::NodeRef tree_ref{tree};
   tree_ref |= ryml::MAP;
   this->serialize_yaml(tree_ref);

   const auto str = ryml::emitrs_yaml<std::string>(tree);
   return (*file)->write({reinterpret_cast<const u8*>(str.data()), str.size()}).has_value();
}

}// namespace triglav::world