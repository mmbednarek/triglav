#include "Project.hpp"
#include "Name.hpp"

#include "triglav/io/File.hpp"
#include "triglav/json_util/Deserialize.hpp"

namespace triglav::project {

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

std::optional<ProjectConfig> load_project_config()
{
   const auto dir = io::home_directory();
   if (!dir.has_value()) {
      return std::nullopt;
   }

   const auto projects_json = dir->sub(".triglav").sub("projects.json");
   const auto project_file = io::open_file(projects_json, io::FileMode::Read);
   if (!project_file.has_value())
      return std::nullopt;

   ProjectConfig config;
   if (!json_util::deserialize(config.to_meta_ref(), **project_file))
      return std::nullopt;

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

std::optional<ProjectInfo> load_project_info(const std::string_view proj_name)
{
   const auto config = load_project_config();
   if (!config.has_value()) {
      return std::nullopt;
   }

   const auto project = std::ranges::find_if(config->projects, [proj_name](const ProjectInfo& info) { return info.name == proj_name; });
   if (project == config->projects.end()) {
      return std::nullopt;
   }

   return *project;
}

std::optional<ProjectInfo> load_current_project_info()
{
   static std::optional<ProjectInfo> cached_md;

   if (cached_md.has_value()) {
      return cached_md;
   }

   cached_md = load_project_info(project_name());
   return cached_md;
}

std::optional<ProjectMetadata> load_project_metadata(const std::string_view name)
{
   const auto proj_info = load_project_info(name);
   if (!proj_info.has_value())
      return std::nullopt;

   const auto projects_json = proj_info->system_path("project.json");
   const auto project_file = io::open_file(projects_json, io::FileMode::Read);
   if (!project_file.has_value())
      return std::nullopt;

   ProjectMetadata result;
   if (!json_util::deserialize(result.to_meta_ref(), **project_file))
      return std::nullopt;

   return result;
}

std::optional<ProjectMetadata> load_current_project_metadata()
{
   static std::optional<ProjectMetadata> cached_md;

   if (cached_md.has_value()) {
      return cached_md;
   }

   cached_md = load_project_metadata(project_name());
   return cached_md;
}

std::optional<ProjectInfo> load_engine_project_info()
{
   static std::optional<ProjectInfo> cached_info;

   if (cached_info.has_value()) {
      return cached_info;
   }

   const auto proj_md = load_current_project_metadata();
   if (!proj_md.has_value())
      return std::nullopt;

   cached_info = load_project_info(proj_md->engine);
   return cached_info;
}

std::optional<ProjectMetadata> load_engine_metadata()
{
   static std::optional<ProjectMetadata> cached_md;

   if (cached_md.has_value()) {
      return cached_md;
   }

   const auto proj_md = load_current_project_metadata();
   if (!proj_md.has_value())
      return std::nullopt;

   cached_md = load_project_metadata(proj_md->engine);
   return cached_md;
}

}// namespace triglav::project

#define TG_NAMESPACE(NS, TYPE_NAME) NS(NS(triglav, project), TYPE_NAME)

#define TG_TYPE(NS) TG_NAMESPACE(NS, ProjectType)
TG_META_ENUM_BEGIN
TG_META_ENUM_VALUE(Engine)
TG_META_ENUM_VALUE(Tool)
TG_META_ENUM_VALUE(Game)
TG_META_ENUM_END
#undef TG_TYPE

#define TG_TYPE(NS) TG_NAMESPACE(NS, ProjectMetadata)
TG_META_CLASS_BEGIN
TG_META_PROPERTY(name, std::string)
TG_META_PROPERTY(identifier, std::string)
TG_META_PROPERTY(type, triglav::project::ProjectType)
TG_META_PROPERTY(engine, std::string)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) TG_NAMESPACE(NS, ImportSettings)
TG_META_CLASS_BEGIN
TG_META_PROPERTY(texture_path, std::string)
TG_META_PROPERTY(mesh_path, std::string)
TG_META_PROPERTY(level_path, std::string)
TG_META_PROPERTY(material_path, std::string)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) TG_NAMESPACE(NS, ProjectInfo)
TG_META_CLASS_BEGIN
TG_META_PROPERTY(name, std::string)
TG_META_PROPERTY(path, std::string)
TG_META_PROPERTY(import_settings, triglav::project::ImportSettings)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) TG_NAMESPACE(NS, ProjectConfig)
TG_META_CLASS_BEGIN
TG_META_PROPERTY(active_project, std::string)
TG_META_ARRAY_PROPERTY(projects, triglav::project::ProjectInfo)
TG_META_CLASS_END
#undef TG_TYPE

#undef TG_NAMESPACE
