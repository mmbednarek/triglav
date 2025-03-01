#include "ProjectConfig.hpp"

namespace triglav::tool::cli {

void ImportSettings::deserialize(const rapidjson::Value& value)
{
   this->texturePath = value["texture_path"].GetString();
   this->meshPath = value["mesh_path"].GetString();
}

void ProjectInfo::deserialize(const rapidjson::Value& value)
{
   this->name = value["name"].GetString();
   this->fullName = value["fullname"].GetString();
   this->path = value["path"].GetString();
   this->importSettings.deserialize(value["import_settings"]);
}

void ProjectConfig::deserialize(const rapidjson::Value& value)
{
   this->activeProject = value["active_project"].GetString();
   for (const auto& child : value["projects"].GetArray()) {
      ProjectInfo info;
      info.deserialize(child);
      this->projects.push_back(info);
   }
}

std::optional<ProjectConfig> load_project_config()
{
   auto dir = io::home_directory();
   if (!dir.has_value()) {
      return std::nullopt;
   }

   auto projectsJson = dir->sub(".triglav").sub("projects.json");

   auto doc = json_util::create_document_from_file(projectsJson);
   if (!doc.has_value()) {
      return std::nullopt;
   }

   ProjectConfig config;
   config.deserialize(*doc);

   return config;
}

std::optional<ProjectInfo> load_active_project_info()
{
   const auto config = load_project_config();
   if (!config.has_value()) {
      return std::nullopt;
   }

   const auto project =
      std::ranges::find_if(config->projects, [&target = config->activeProject](const ProjectInfo& info) { return info.name == target; });
   if (project == config->projects.end()) {
      return std::nullopt;
   }

   return *project;
}

}// namespace triglav::tool::cli
