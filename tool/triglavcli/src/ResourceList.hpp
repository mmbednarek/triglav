#pragma once

#include "triglav/io/Path.hpp"

#include <string>
#include <map>
#include <optional>
#include <ryml.hpp>

namespace triglav::tool::cli {

struct ResourceListItem
{
   std::string source;
   std::map<std::string, std::string> properties;

   void deserialize_yaml(const ryml::ConstNodeRef& node);
   void serialize_yaml(ryml::NodeRef& node) const;
};

struct ResourceList
{
   int version;
   std::map<std::string, ResourceListItem> resources;

   void deserialize_yaml(const ryml::ConstNodeRef& node);
   void serialize_yaml(ryml::NodeRef& node) const;

   [[nodiscard]] static std::optional<ResourceList> from_file(const io::Path& path);
   [[nodiscard]] bool save_to_file(const io::Path& path) const;
};

bool add_resource_to_index(std::string_view name);

}