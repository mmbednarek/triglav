#pragma once

#include "triglav/json_util/JsonUtil.hpp"
#include "triglav/ResourceType.hpp"

#include <string>
#include <vector>
#include <optional>

namespace triglav::tool::cli {

struct ImportSettings
{
   std::string texturePath;
   std::string meshPath;
   std::string levelPath;

   void deserialize(const rapidjson::Value& value);
};

struct ProjectInfo
{
   std::string name;
   std::string fullName;
   std::string path;
   ImportSettings importSettings;

   void deserialize(const rapidjson::Value& value);
   [[nodiscard]] io::Path system_path(std::string_view resourcePath) const;
   [[nodiscard]] io::Path content_path(std::string_view resourcePath) const;
   [[nodiscard]] std::string default_import_path(ResourceType resType, std::string_view basename) const;
};

struct ProjectConfig
{
   std::string activeProject{};
   std::vector<ProjectInfo> projects{};

   void deserialize(const rapidjson::Value& value);
};

[[nodiscard]] std::optional<ProjectConfig> load_project_config();
[[nodiscard]] std::optional<ProjectInfo> load_active_project_info();

}// namespace triglav::tool::cli