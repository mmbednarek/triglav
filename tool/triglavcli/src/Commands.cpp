#include "Commands.hpp"

#include <cassert>
#include <fmt/core.h>

namespace triglav::tool::cli {

constexpr auto g_argDescOffset = 20;

namespace {
void print_argument(std::string_view shorthand, std::string_view longname, std::string_view description)
{
   assert(shorthand.length() < g_argDescOffset);
   fmt::print(stderr, "   -{}, --{}", shorthand, longname);
   const auto remainingOffset = g_argDescOffset - longname.size();
   for (auto i = 0u; i < remainingOffset; i++) {
      fmt::print(stderr, " ");
   }
   fmt::print(stderr, "{}\n", description);
}

}// namespace

std::optional<Command> command_from_string(const std::string_view argName)
{
#define TG_DECLARE_COMMAND(name, desc) \
   if (argName == #name) {             \
      return Command::name;            \
   }
#include "ProgramOptions.def"


   return std::nullopt;
}

#define TG_DECLARE_COMMAND(name, desc)                           \
   bool CmdArgs_##name::parse(const u32 argc, const char** argv) \
   {                                                             \
      std::string currentOpt{};                                  \
      for (u32 i = 2; i < argc; ++i) {                           \
         const std::string_view arg{argv[i]};                    \
         if (!arg.starts_with("-")) {                            \
            this->positionalArgs.emplace_back(arg);              \
            continue;                                            \
         }

#define TG_END_COMMAND()                                  \
   fmt::print(stderr, "unrecognized option '{}'\n", arg); \
   return false;                                          \
   }                                                      \
   return true;                                           \
   }

#define TG_PARSE_ARG_StringArray(argName) this->argName.emplace_back(optionValue);
#define TG_PARSE_ARG_String(argName) this->argName = optionValue;
#define TG_PARSE_ARG_Int(argName) this->argName = std::stoi(optionValue);

#define TG_DECLARE_ARG(argName, shorthand, longname, type, description) \
   if (arg == "-" shorthand || arg == "--" longname) {                  \
      ++i;                                                              \
      if (i >= argc) {                                                  \
         fmt::print("Expected argument missing for " #argName "\n");    \
         return false;                                                  \
      }                                                                 \
      std::string_view optionValue{argv[i]};                            \
      TG_PARSE_ARG_##type(argName) continue;                            \
   }


#define TG_DECLARE_FLAG(argName, shorthand, longname, description) \
   if (arg == "-" shorthand || arg == "--" longname) {             \
      argName = true;                                              \
      continue;                                                    \
   }

#include "ProgramOptions.def"

#define TG_DECLARE_COMMAND(name, desc)                                    \
   void CmdArgs_##name::print_help()                                      \
   {                                                                      \
      fmt::print(stderr, "Triglav CLI Tool - " #name "\n\n" desc "\n\n"); \
      fmt::print(stderr, "OPTIONS\n");


#define TG_END_COMMAND() }

#define TG_DECLARE_ARG(argName, shorthand, longname, type, description) print_argument(shorthand, longname, description);

#define TG_DECLARE_FLAG(argName, shorthand, longname, description) print_argument(shorthand, longname, description);

#include "ProgramOptions.def"

}// namespace triglav::tool::cli
