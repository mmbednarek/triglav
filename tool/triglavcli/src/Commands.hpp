#pragma once

#include <string>

namespace triglav::tool::cli {

using ExitStatus = int;

enum class Command
{
   Run,
   Import,
};

struct ImportArgs
{
   std::string srcAsset;
   std::string dstPath;
};

ExitStatus handle_import(const ImportArgs& args);

}