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
   this->texture_path = value["texture_path"].GetString();
   this->mesh_path = value["mesh_path"].GetString();
   this->level_path = value["level_path"].GetString();
   this->material_path = value["material_path"].GetString();
}

void ProjectInfo::deserialize(const rapidjson::Value& value)
{
   this->name = value["name"].GetString();
   this->full_name = value["fullname"].GetString();
   this->path = value["path"].GetString();
   this->import_settings.deserialize(value["import_settings"]);
}

io::Path ProjectInfo::system_path(const std::string_view resource_path) const
{
   const std::string sub_path = to_system_path(resource_path);
   const io::Path project_path{this->path};
   return project_path.sub(sub_path);
}

io::Path ProjectInfo::content_path(const std::string_view resource_path) const
{
   const std::string sub_path = to_system_path(resource_path);
   const io::Path project_path{this->path};
   return project_path.sub("content").sub(sub_path);
}

std::string ProjectInfo::default_import_path(const ResourceType res_type, const std::string_view basename) const
{
   std::string result;
   switch (res_type) {
   case ResourceType::Texture:
      result = this->import_settings.texture_path;
      break;
   case ResourceType::Mesh:
      result = this->import_settings.mesh_path;
      break;
   case ResourceType::Level:
      result = this->import_settings.level_path;
      break;
   case ResourceType::Material:
      result = this->import_settings.material_path;
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
   this->active_project = value["active_project"].GetString();
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

   auto projects_json = dir->sub(".triglav").sub("projects.json");

   auto doc = json_util::create_document_from_file(projects_json);
   if (!doc.has_value()) {
      return std::nullopt;
   }

   ProjectConfig config;
   config.deserialize(*doc);

   return config;
}

std::optional<ProjectInfo> load_active_project_info()
{
   static std::optional<ProjectInfo> cached_config;

   if (cached_config.has_value()) {
      return cached_config;
   }

   const auto config = load_project_config();
   if (!config.has_value()) {
      return std::nullopt;
   }

   const auto project =
      std::ranges::find_if(config->projects, [&target = config->active_project](const ProjectInfo& info) { return info.name == target; });
   if (project == config->projects.end()) {
      return std::nullopt;
   }

   cached_config.emplace(*project);

   return *project;
}

}// namespace triglav::tool::cli
