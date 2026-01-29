#include "Commands.hpp"

#include "triglav/io/File.hpp"
#include "triglav/io/Path.hpp"
#include "triglav/json_util/Deserialize.hpp"
#include "triglav/project/ProjectManager.hpp"

#include <print>
#include <vector>

namespace triglav::tool::cli {

void discover_projects(const io::Path& path)
{
   for (const auto [name, is_dir] : io::list_files(path)) {
      if (is_dir) {
         discover_projects(path.sub(name.to_std()));
      } else if (name == "project.json") {
         const auto project_path = path.sub(name.to_std());
         auto project_file = io::open_file(project_path, io::FileMode::Read);
         if (!project_file.has_value())
            continue;

         project::ProjectMetadata metadata;
         if (!json_util::deserialize(metadata.to_meta_ref(), **project_file))
            continue;

         std::print(stderr, "Discovered project {} at {}\n", metadata.identifier, project_path.string());

         project::ProjectManager::the().add_project(metadata.identifier, path);
      }
   }
}

ExitStatus handle_project(const CmdArgs_project& args)
{
   std::print(stderr, "Active project: {}\n", project::ProjectManager::the().active_project_identifier());

   if (args.should_list) {
      for (const auto& proj : project::ProjectManager::the().projects()) {
         const auto& metadata = project::ProjectManager::the().project_metadata(proj.name_id);

         std::print(stderr, "-- {} --\n", metadata->name);
         std::print(stderr, "  Identifier: {}\n", proj.name);
         std::print(stderr, "  Path: {}\n", proj.path);
         std::print(stderr, "  Engine: {}\n\n", metadata->engine);
      }
   }

   if (args.discover) {
      const auto wd = io::working_path();
      if (!wd.has_value()) {
         std::print(stderr, "triglav-cli: Failed to obtain working directory.\n");
         return EXIT_FAILURE;
      }

      discover_projects(*wd);
      if (!project::ProjectManager::the().save_project_info()) {
         std::print(stderr, "triglav-cli: Failed to saved project info.\n");
         return EXIT_FAILURE;
      }
   }

   if (!args.set_active.empty()) {
      if (!project::ProjectManager::the().set_active_project(args.set_active)) {
         std::print(stderr, "triglav-cli: Failed to set active project.\n");
         return EXIT_FAILURE;
      }
      if (!project::ProjectManager::the().save_project_info()) {
         std::print(stderr, "triglav-cli: Failed to saved project info.\n");
         return EXIT_FAILURE;
      }
   }

   return EXIT_SUCCESS;
}

}// namespace triglav::tool::cli
