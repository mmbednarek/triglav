#include "Commands.hpp"
#include "triglav/gltf/Glb.hpp"
#include "triglav/io/File.hpp"
#include "triglav/io/Path.hpp"

#include <iostream>
#include <print>

namespace triglav::tool::cli {

ExitStatus handle_glb_json_extract(const CmdArgs_glb_json_extract& args)
{
   if (args.positional_args.size() < 2) {
      std::println(std::cerr, "triglav-cli: not enough arguments");
      return EXIT_FAILURE;
   }

   const auto json_file = io::open_file(io::Path(args.positional_args[1]), io::FileMode::Write | io::FileMode::Create);
   if (!json_file.has_value()) {
      std::println(std::cerr, "triglav-cli: cannot open destination file");
      return EXIT_FAILURE;
   }

   const auto res = gltf::extract_glb_json(io::Path(args.positional_args[0]), **json_file);
   if (!res.has_value()) {
      std::println(std::cerr, "triglav-cli: failed to extract glb");
      return EXIT_FAILURE;
   }

   std::println(std::cerr, "triglav-cli: written {} bytes to \"{}\"", *res, args.positional_args[1]);
   return EXIT_SUCCESS;
}

}// namespace triglav::tool::cli