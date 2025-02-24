#include "Commands.hpp"

#include "triglav/Int.hpp"

#include <cstdlib>
#include <fmt/core.h>
#include <optional>
#include <string>
#include <string_view>

using triglav::tool::cli::Command;
using triglav::tool::cli::ExitStatus;

ExitStatus main(const int argc, const char** argv)
{
   if (argc < 2) {
      fmt::print(stderr, "triglav-cli: not enough arguments\n\n");
      triglav::tool::cli::handle_help({});
      return EXIT_FAILURE;
   }

   const auto command = triglav::tool::cli::command_from_string(argv[1]);
   if (!command.has_value()) {
      fmt::print(stderr, "triglav-cli: unknown command '{}'\n\n", argv[1]);
      triglav::tool::cli::handle_help({});
      return EXIT_FAILURE;
   }

   switch (*command) {
#define TG_DECLARE_COMMAND(name, desc)                          \
   case Command::name: {                                        \
      triglav::tool::cli::CmdArgs_##name args{};                \
      if (argc == 3 && std::string_view{argv[2]} == "--help") { \
         args.print_help();                                     \
         return EXIT_SUCCESS;                                   \
      } else if (!args.parse(argc, argv)) {                     \
         return EXIT_FAILURE;                                   \
      }                                                         \
      return handle_##name(args);                               \
   }
#include "ProgramOptions.def"

   default:
      return EXIT_FAILURE;
   }
}