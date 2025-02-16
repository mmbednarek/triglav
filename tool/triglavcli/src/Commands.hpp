#pragma once

#include <string>

namespace triglav::tool::cli {

using ExitStatus = int;

enum class Command
{
   Run,
   Import,
   Inspect,
};

struct ImportArgs
{
   std::string srcAsset;
   std::string dstPath;
};

struct InspectArgs
{
   std::string inputAccess;
};

ExitStatus handle_import(const ImportArgs& args);
ExitStatus handle_inspect(const InspectArgs& args);

}