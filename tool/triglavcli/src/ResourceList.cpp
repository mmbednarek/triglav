#include "ResourceList.hpp"

#include "ProjectConfig.hpp"

#include "triglav/io/File.hpp"

#include <fmt/core.h>

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

void ResourceListItem::deserialize_yaml(const ryml::ConstNodeRef& node)
{
   const auto sourceVal = node["source"].val();

   this->source = {sourceVal.data(), sourceVal.size()};

   if (node.has_child("properties")) {
      auto propertiesNode = node["properties"];
      for (const auto property : propertiesNode) {
         auto key = property.key();
         auto value = property.val();
         this->properties.emplace(std::string{key.data(), key.size()}, std::string{value.data(), value.size()});
      }
   }
}
void ResourceListItem::serialize_yaml(ryml::NodeRef& node) const
{
   node["source"] << ryml::csubstr(this->source.data(), this->source.size());

   if (!this->properties.empty()) {
      auto propertiesNode = node["properties"];
      propertiesNode |= ryml::MAP;

      for (const auto& [key, value] : this->properties) {
         propertiesNode[ryml::csubstr(key.data(), key.size())] = ryml::csubstr(value.data(), value.size());
      }
   }
}

void ResourceList::deserialize_yaml(const ryml::ConstNodeRef& node)
{
   const auto versionStr = node["version"].val();
   this->version = std::stoi(std::string{versionStr.data(), versionStr.size()});

   const auto resourcesNode = node["resources"];
   for (const auto resource : resourcesNode) {
      const auto nameStr = resource["name"].val();
      std::string name = {nameStr.data(), nameStr.size()};

      ResourceListItem item;
      item.deserialize_yaml(resource);
      this->resources.emplace(std::move(name), std::move(item));
   }
}

void ResourceList::serialize_yaml(ryml::NodeRef& node) const
{
   const auto versionStr = std::to_string(this->version);
   node["version"] << ryml::csubstr(versionStr.data(), versionStr.size());

   auto resourcesNode = node["resources"];
   resourcesNode |= ryml::SEQ;
   for (const auto& [name, resource] : this->resources) {

      auto child = resourcesNode.append_child();
      child |= ryml::MAP;

      child["name"] << ryml::csubstr(name.data(), name.size());
      resource.serialize_yaml(child);
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
   const auto file = io::open_file(path, io::FileOpenMode::Create);
   if (!file.has_value()) {
      return false;
   }

   ryml::Tree tree;
   ryml::NodeRef treeRef{tree};
   treeRef |= ryml::MAP;
   this->serialize_yaml(treeRef);

   const auto str = ryml::emitrs_yaml<std::string>(tree);
   return (*file)->write({reinterpret_cast<const u8*>(str.data()), str.size()}).has_value();
}

bool add_resource_to_index(const std::string_view name)
{
   auto projectInfo = load_active_project_info();
   if (!projectInfo.has_value()) {
      fmt::print(stderr, "triglav-cli: No active project found\n");
      return false;
   }

   const auto indexPath = projectInfo->content_path("index.yaml");
   auto resList = ResourceList::from_file(indexPath);
   assert(resList.has_value());

   resList->resources.emplace(std::string{name}, std::string{name});

   return resList->save_to_file(indexPath);
}

}// namespace triglav::tool::cli
