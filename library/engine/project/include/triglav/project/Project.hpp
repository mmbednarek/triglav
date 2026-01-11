#pragma once

#include "triglav/io/Path.hpp"
#include "triglav/meta/Meta.hpp"

#include <string>
#include <vector>

namespace triglav::project {

enum class ProjectType
{
   Engine,
   Tool,
   Game,
};

struct ProjectMetadata
{
   TG_META_STRUCT_BODY(ProjectMetadata)

   std::string name;
   std::string identifier;
   ProjectType type;
   std::string engine;
};

struct ImportSettings
{
   TG_META_STRUCT_BODY(ImportSettings)

   std::string texture_path;
   std::string mesh_path;
   std::string level_path;
   std::string material_path;
};

struct ProjectInfo
{
   TG_META_STRUCT_BODY(ProjectInfo)

   std::string name;
   std::string full_name;
   std::string path;
   ImportSettings import_settings;

   [[nodiscard]] io::Path system_path(std::string_view resource_path) const;
   [[nodiscard]] io::Path content_path(std::string_view resource_path) const;
   [[nodiscard]] std::string default_import_path(ResourceType res_type, std::string_view basename) const;
};

struct ProjectConfig
{
   TG_META_STRUCT_BODY(ProjectConfig)

   std::string active_project{};
   std::vector<ProjectInfo> projects{};
};

[[nodiscard]] std::optional<ProjectConfig> load_project_config();
[[nodiscard]] std::optional<ProjectInfo> load_active_project_info();
[[nodiscard]] std::optional<ProjectInfo> load_current_project_info();
[[nodiscard]] std::optional<ProjectInfo> load_engine_project_info();
[[nodiscard]] std::optional<ProjectInfo> load_project_info(std::string_view proj_name);
[[nodiscard]] std::optional<ProjectMetadata> load_project_metadata(std::string_view name);
[[nodiscard]] std::optional<ProjectMetadata> load_current_project_metadata();
[[nodiscard]] std::optional<ProjectMetadata> load_engine_metadata();

}// namespace triglav::project
