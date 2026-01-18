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
   [[nodiscard]] Name active_project() const;
   [[nodiscard]] std::string_view active_project_identifier() const;
   [[nodiscard]] const std::vector<ProjectInfo>& projects() const;
   [[nodiscard]] const ProjectInfo* active_project_info() const;
   [[nodiscard]] const ProjectMetadata* active_project_metadata();
   void add_project(std::string_view project_name, const io::Path& project_path);
   bool set_active_project(std::string_view project_name);

   [[nodiscard]] bool save_project_info();

   static ProjectManager& the();

 private:
   ProjectConfig m_config;
   std::map<Name, ProjectMetadata> m_metadata;
};

Name this_project();
Name game_project();
Name engine_project();

}// namespace triglav::project