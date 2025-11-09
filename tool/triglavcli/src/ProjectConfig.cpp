#include "ProjectConfig.hpp"

#include <format>
#include <ranges>

namespace triglav::tool::cli {

namespace {
std::string to_system_path(const std::string_view path)
{
   if (io::path_seperator() == '/')
      return std::string{path};

   std::string result{path};
   for (auto& ch : result) {
      if (ch == '/') {
         ch = io::path_seperator();
      }
   }
   return result;
}
}// namespace

void ImportSettings::deserialize(const rapidjson::Value& value)
{
   this->texturePath = value["texture_path"].GetString();
   this->meshPath = value["mesh_path"].GetString();
   this->levelPath = value["level_path"].GetString();
   this->materialPath = value["material_path"].GetString();
}

void ProjectInfo::deserialize(const rapidjson::Value& value)
{
   this->name = value["name"].GetString();
   this->fullName = value["fullname"].GetString();
   this->path = value["path"].GetString();
   this->importSettings.deserialize(value["import_settings"]);
}

io::Path ProjectInfo::system_path(const std::string_view resourcePath) const
{
   const std::string subPath = to_system_path(resourcePath);
   const io::Path projectPath{this->path};
   return projectPath.sub(subPath);
}

io::Path ProjectInfo::content_path(const std::string_view resourcePath) const
{
   const std::string subPath = to_system_path(resourcePath);
   const io::Path projectPath{this->path};
   return projectPath.sub("content").sub(subPath);
}

std::string ProjectInfo::default_import_path(const ResourceType resType, const std::string_view basename) const
{
   std::string result;
   switch (resType) {
   case ResourceType::Texture:
      result = this->importSettings.texturePath;
      break;
   case ResourceType::Mesh:
      result = this->importSettings.meshPath;
      break;
   case ResourceType::Level:
      result = this->importSettings.levelPath;
      break;
   case ResourceType::Material:
      result = this->importSettings.materialPath;
      break;
   default:
      return "";
   }

   auto index = result.find("{basename}");
   while (index != std::string::npos) {
      result.replace(index, 10, basename);
      index = result.find("{basename}");
   }
   return result;
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
   static std::optional<ProjectInfo> cachedConfig;

   if (cachedConfig.has_value()) {
      return cachedConfig;
   }

   const auto config = load_project_config();
   if (!config.has_value()) {
      return std::nullopt;
   }

   const auto project =
      std::ranges::find_if(config->projects, [&target = config->activeProject](const ProjectInfo& info) { return info.name == target; });
   if (project == config->projects.end()) {
      return std::nullopt;
   }

   cachedConfig.emplace(*project);

   return *project;
}

}// namespace triglav::tool::cli
