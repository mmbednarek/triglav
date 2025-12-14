#pragma once

#include "triglav/io/Path.hpp"

#include <optional>
#include <ryml.hpp>
#include <string>
#include <vector>

namespace triglav::tool::cli {

struct ResourceList
{
   int version;
   std::vector<std::string> resources;

   void deserialize_yaml(const ryml::ConstNodeRef& node);
   void serialize_yaml(ryml::NodeRef& node) const;

   [[nodiscard]] static std::optional<ResourceList> from_file(const io::Path& path);
   [[nodiscard]] bool save_to_file(const io::Path& path) const;
};

bool add_resource_to_index(std::string_view name);

}// namespace triglav::tool::cli