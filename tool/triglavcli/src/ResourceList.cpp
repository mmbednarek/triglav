#include "ResourceList.hpp"

#include "ProjectConfig.hpp"

#include "triglav/io/File.hpp"

#include <print>

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

namespace triglav::tool::cli {

void ResourceList::deserialize_yaml(const ryml::ConstNodeRef& node)
{
   const auto version_str = node["version"].val();
   this->version = std::stoi(std::string{version_str.data(), version_str.size()});

   const auto resources_node = node["resources"];
   for (const auto resource : resources_node) {
      const auto name_str = resource.val();
      this->resources.push_back({name_str.data(), name_str.size()});
   }
}

void ResourceList::serialize_yaml(ryml::NodeRef& node) const
{
   const auto version_str = std::to_string(this->version);
   node["version"] << ryml::csubstr(version_str.data(), version_str.size());

   auto resources_node = node["resources"];
   resources_node |= ryml::SEQ;
   for (const auto& rc_path : this->resources) {
      auto child = resources_node.append_child();
      child << ryml::csubstr(rc_path.data(), rc_path.size());
   }
}

std::optional<ResourceList> ResourceList::from_file(const io::Path& path)
{
   auto content = io::read_whole_file(path);
   if (content.empty()) {
      return std::nullopt;
   }

   const auto tree = ryml::parse_in_place(c4::substr{const_cast<char*>(path.string().data()), path.string().size()},
                                          c4::substr{content.data(), content.size()});
   ResourceList result;
   result.deserialize_yaml(tree);

   return result;
}

bool ResourceList::save_to_file(const io::Path& path) const
{
   const auto file = io::open_file(path, io::FileMode::Write | io::FileMode::Create);
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

bool add_resource_to_index(const std::string_view name)
{
   auto project_info = load_active_project_info();
   if (!project_info.has_value()) {
      std::print(stderr, "triglav-cli: No active project found\n");
      return false;
   }

   const auto index_path = project_info->content_path("index.yaml");
   auto res_list = ResourceList::from_file(index_path);
   assert(res_list.has_value());

   res_list->resources.emplace_back(name);

   return res_list->save_to_file(index_path);
}

}// namespace triglav::tool::cli
