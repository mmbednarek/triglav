#pragma once

#include "triglav/ResourceType.hpp"
#include "triglav/json_util/JsonUtil.hpp"

#include <optional>
#include <string>
#include <vector>

namespace triglav::tool::cli {

struct ImportSettings
{
   std::string texture_path;
   std::string mesh_path;
   std::string level_path;
   std::string material_path;

   void deserialize(const rapidjson::Value& value);
};

struct ProjectInfo
{
   std::string name;
   std::string full_name;
   std::string path;
   ImportSettings import_settings;

   void deserialize(const rapidjson::Value& value);
   [[nodiscard]] io::Path system_path(std::string_view resource_path) const;
   [[nodiscard]] io::Path content_path(std::string_view resource_path) const;
   [[nodiscard]] std::string default_import_path(ResourceType res_type, std::string_view basename) const;
};

struct ProjectConfig
{
   std::string active_project{};
   std::vector<ProjectInfo> projects{};

   void deserialize(const rapidjson::Value& value);
};

[[nodiscard]] std::optional<ProjectConfig> load_project_config();
[[nodiscard]] std::optional<ProjectInfo> load_active_project_info();

}// namespace triglav::tool::cli