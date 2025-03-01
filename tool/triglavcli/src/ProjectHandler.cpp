#include "Commands.hpp"

#include "ProjectConfig.hpp"

#include "triglav/io/Path.hpp"
#include "triglav/json_util/JsonUtil.hpp"

#include <fmt/core.h>
#include <vector>

namespace triglav::tool::cli {

ExitStatus handle_project(const CmdArgs_project& args)
{
   auto config = load_project_config();
   if (!config.has_value())
      return EXIT_FAILURE;

   if (args.shouldList) {
      fmt::print(stderr, "Active project: {}\n", config->activeProject);
      for (const auto& project : config->projects) {
         fmt::print(stderr, "  Project: {}\n", project.name);
         fmt::print(stderr, "  Full Name: {}\n", project.fullName);
         fmt::print(stderr, "  Path: {}\n", project.path);
      }
   }

   return EXIT_SUCCESS;
}

}// namespace triglav::tool::cli
