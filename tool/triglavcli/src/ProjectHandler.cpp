#include "Commands.hpp"

#include "ProjectConfig.hpp"

#include "triglav/io/Path.hpp"
#include "triglav/json_util/JsonUtil.hpp"

#include <print>
#include <vector>

namespace triglav::tool::cli {

ExitStatus handle_project(const CmdArgs_project& args)
{
   auto config = load_project_config();
   if (!config.has_value())
      return EXIT_FAILURE;

   if (args.should_list) {
      std::print(stderr, "Active project: {}\n", config->active_project);
      for (const auto& project : config->projects) {
         std::print(stderr, "  Project: {}\n", project.name);
         std::print(stderr, "  Full Name: {}\n", project.full_name);
         std::print(stderr, "  Path: {}\n", project.path);
      }
   }

   return EXIT_SUCCESS;
}

}// namespace triglav::tool::cli
