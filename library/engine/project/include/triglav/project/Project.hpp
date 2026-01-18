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
   Test,
};

struct ResourcePathMapping
{
   TG_META_STRUCT_BODY(ResourcePathMapping)

   std::string engine_path;
   std::string system_path;
};

struct ImportSettings
{
   TG_META_STRUCT_BODY(ImportSettings)

   std::string texture_path;
   std::string mesh_path;
   std::string level_path;
   std::string material_path;
};

struct ProjectMetadata
{
   TG_META_STRUCT_BODY(ProjectMetadata)

   std::string name;
   std::string identifier;
   ProjectType type;
   std::string engine;
   std::vector<ResourcePathMapping> resource_mapping;
   ImportSettings import_settings;

   [[nodiscard]] std::string default_import_path(ResourceType res_type, std::string_view basename) const;
};

struct ProjectInfo
{
   TG_META_STRUCT_BODY(ProjectInfo)

   std::string name;
   std::string path;

   // meta: hidden
   Name name_id;
};

struct ProjectConfig
{
   TG_META_STRUCT_BODY(ProjectConfig)

   std::string active_project{};
   std::vector<ProjectInfo> projects{};
};

}// namespace triglav::project
