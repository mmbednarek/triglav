#include "Commands.hpp"

#include <cassert>
#include <format>
#include <print>

namespace triglav::tool::cli {

constexpr auto g_descOffset = 16;

namespace {

void print_command(std::string_view commandName, std::string_view description)
{
   assert(commandName.length() < g_descOffset);
   std::print(stderr, "   {}", commandName);
   const auto remainingOffset = g_descOffset - commandName.size();
   for (auto i = 0u; i < remainingOffset; i++) {
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