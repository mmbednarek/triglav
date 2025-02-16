#include "Commands.hpp"

#include <cstdlib>
#include <fmt/core.h>
#include <optional>
#include <string>
#include <string_view>

using triglav::tool::cli::Command;
using triglav::tool::cli::ExitStatus;
using triglav::tool::cli::ImportArgs;
using triglav::tool::cli::InspectArgs;

std::optional<Command> command_from_string(const std::string_view name)
{
   if (name == "run") {
      return Command::Run;
   }
   if (name == "import") {
      return Command::Import;
   }
   if (name == "inspect") {
      return Command::Inspect;
   }
   return std::nullopt;
}

ImportArgs parse_import_args(const int argc, const char** argv)
{
   if (argc < 4) {
      return ImportArgs{};
   }
   return ImportArgs{argv[2], argv[3]};
}

InspectArgs parse_inspect_args(const int argc, const char** argv)
{
   if (argc < 3) {
      return InspectArgs{};
   }
   return InspectArgs{argv[2]};
}

ExitStatus main(const int argc, const char** argv)
{
   if (argc < 2) {
      fmt::print(stderr, "not enough arguments\n");
      return EXIT_FAILURE;
   }

   const auto command = command_from_string(argv[1]);
   if (!command.has_value()) {
      fmt::print(stderr, "unknown command\n");
      return EXIT_FAILURE;
   }

   switch (*command) {
   case Command::Run:
      fmt::print(stderr, "unimplemented\n");
      return EXIT_SUCCESS;
   case Command::Import:
      return handle_import(parse_import_args(argc, argv));
   case Command::Inspect:
      return handle_inspect(parse_inspect_args(argc, argv));
   default:
      return EXIT_FAILURE;
   }
}