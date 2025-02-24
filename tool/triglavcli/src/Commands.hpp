#pragma once

#include "triglav/Int.hpp"

#include <string>
#include <vector>
#include <optional>

namespace triglav::tool::cli {

using ExitStatus = int;

#define TG_DECLARE_COMMAND(name, desc) name,

enum class Command
{
#include "ProgramOptions.def"
};

std::optional<Command> command_from_string(std::string_view argName);

#define TG_ARG_TYPE_StringArray std::vector<std::string>
#define TG_ARG_TYPE_String std::string
#define TG_ARG_TYPE_Int int

#define TG_DECLARE_COMMAND(name, desc) struct CmdArgs_##name { \
     std::vector<std::string> positionalArgs{};
#define TG_END_COMMAND() \
      bool parse(const u32 argc, const char** argv); \
      void print_help(); \
   };
#define TG_DECLARE_ARG(argName, shorthand, longname, type, description) TG_ARG_TYPE_##type argName{};
#define TG_DECLARE_FLAG(argName, shorthand, longname, description) bool argName{};

#include "ProgramOptions.def"

#undef TG_ARG_TYPE_StringArray
#undef TG_ARG_TYPE_String
#undef TG_ARG_TYPE_Int

#define TG_DECLARE_COMMAND(name, desc) ExitStatus handle_##name(const CmdArgs_##name& args);

#include "ProgramOptions.def"

}// namespace triglav::tool::cli