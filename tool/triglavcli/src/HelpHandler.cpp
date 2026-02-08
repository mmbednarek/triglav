#include "Commands.hpp"

#include <cassert>
#include <format>
#include <print>

namespace triglav::tool::cli {

constexpr auto g_desc_offset = 32;

namespace {

void print_command(std::string_view command_name, std::string_view description)
{
   assert(command_name.length() < g_desc_offset);
   std::print(stderr, "   {}", command_name);
   const auto remaining_offset = g_desc_offset - command_name.size();
   for (auto i = 0u; i < remaining_offset; i++) {
      std::print(stderr, " ");
   }
   std::print(stderr, "{}\n", description);
}

}// namespace

ExitStatus handle_help(const CmdArgs_help& /*args*/)
{
   std::print(stderr, "Triglav CLI tool is used to create and maintain triglav projects.\n\nCOMMANDS\n");

#define TG_DECLARE_COMMAND(name, desc) print_command(#name, desc);

#include "ProgramOptions.def"


   return EXIT_SUCCESS;
}

}// namespace triglav::tool::cli