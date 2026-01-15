#pragma once

#include "Project.hpp"

#include <map>
#include <string_view>

namespace triglav::project {

class ProjectManager
{
 public:
   ProjectManager();

   [[nodiscard]] const ProjectInfo* project_info(Name project_name) const;
   [[nodiscard]] std::string_view project_root(Name project_name) const;
   [[nodiscard]] const ProjectMetadata* project_metadata(Name project_name);

   static ProjectManager& the();

 private:
   ProjectConfig m_config;
   std::map<Name, ProjectMetadata> m_metadata;
};

Name this_project();
Name game_project();
Name engine_project();

}// namespace triglav::project