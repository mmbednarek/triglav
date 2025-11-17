#pragma once

#include "triglav/Int.hpp"

#include <optional>
#include <string>
#include <vector>

namespace triglav::tool::cli {

using ExitStatus = int;

#define TG_DECLARE_COMMAND(name, desc) name,

enum class Command
{
#include "ProgramOptions.def"
};

std::optional<Command> command_from_string(std::string_view arg_name);

#define TG_ARG_TYPE_StringArray std::vector<std::string>
#define TG_ARG_TYPE_String std::string
#define TG_ARG_TYPE_Int int

#define TG_DECLARE_COMMAND(name, desc) \
   struct CmdArgs_##name               \
   {                                   \
      std::vector<std::string> positional_args{};
#define TG_END_COMMAND()                          \
   bool parse(const u32 argc, const char** argv); \
   void print_help();                             \
   }                                              \
   ;
#define TG_DECLARE_ARG(arg_name, shorthand, longname, type, description) TG_ARG_TYPE_##type arg_name{};
#define TG_DECLARE_FLAG(arg_name, shorthand, longname, description) bool arg_name{};

#include "ProgramOptions.def"

#undef TG_ARG_TYPE_StringArray
#undef TG_ARG_TYPE_String
#undef TG_ARG_TYPE_Int

#define TG_DECLARE_COMMAND(name, desc) ExitStatus handle_##name(const CmdArgs_##name& args);

#include "ProgramOptions.def"

}// namespace triglav::tool::cli