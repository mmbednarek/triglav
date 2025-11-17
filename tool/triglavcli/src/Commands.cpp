#include "Commands.hpp"

#include <cassert>
#include <format>
#include <print>

namespace triglav::tool::cli {

constexpr auto g_arg_desc_offset = 20;

namespace {
void print_argument(std::string_view shorthand, std::string_view longname, std::string_view description)
{
   assert(shorthand.length() < g_arg_desc_offset);
   std::print(stderr, "   -{}, --{}", shorthand, longname);
   const auto remaining_offset = g_arg_desc_offset - longname.size();
   for (auto i = 0u; i < remaining_offset; i++) {
      std::print(stderr, " ");
   }
   std::print(stderr, "{}\n", description);
}

}// namespace

std::optional<Command> command_from_string(const std::string_view arg_name)
{
#define TG_DECLARE_COMMAND(name, desc) \
   if (arg_name == #name) {            \
      return Command::name;            \
   }
#include "ProgramOptions.def"


   return std::nullopt;
}

#define TG_DECLARE_COMMAND(name, desc)                           \
   bool CmdArgs_##name::parse(const u32 argc, const char** argv) \
   {                                                             \
      std::string current_opt{};                                 \
      for (u32 i = 2; i < argc; ++i) {                           \
         const std::string_view arg{argv[i]};                    \
         if (!arg.starts_with("-")) {                            \
            this->positional_args.emplace_back(arg);             \
            continue;                                            \
         }

#define TG_END_COMMAND()                                  \
   std::print(stderr, "unrecognized option '{}'\n", arg); \
   return false;                                          \
   }                                                      \
   return true;                                           \
   }

#define TG_PARSE_ARG_StringArray(arg_name) this->arg_name.emplace_back(option_value);
#define TG_PARSE_ARG_String(arg_name) this->arg_name = option_value;
#define TG_PARSE_ARG_Int(arg_name) this->arg_name = std::stoi(option_value);

#define TG_DECLARE_ARG(arg_name, shorthand, longname, type, description) \
   if (arg == "-" shorthand || arg == "--" longname) {                   \
      ++i;                                                               \
      if (i >= argc) {                                                   \
         std::print("Expected argument missing for " #arg_name "\n");    \
         return false;                                                   \
      }                                                                  \
      std::string_view option_value{argv[i]};                            \
      TG_PARSE_ARG_##type(arg_name) continue;                            \
   }


#define TG_DECLARE_FLAG(arg_name, shorthand, longname, description) \
   if (arg == "-" shorthand || arg == "--" longname) {              \
      arg_name = true;                                              \
      continue;                                                     \
   }

#include "ProgramOptions.def"

#define TG_DECLARE_COMMAND(name, desc)                                    \
   void CmdArgs_##name::print_help()                                      \
   {                                                                      \
      std::print(stderr, "Triglav CLI Tool - " #name "\n\n" desc "\n\n"); \
      std::print(stderr, "OPTIONS\n");


#define TG_END_COMMAND() }

#define TG_DECLARE_ARG(arg_name, shorthand, longname, type, description) print_argument(shorthand, longname, description);

#define TG_DECLARE_FLAG(arg_name, shorthand, longname, description) print_argument(shorthand, longname, description);

#include "ProgramOptions.def"

}// namespace triglav::tool::cli
