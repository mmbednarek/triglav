#include "triglav/test_util/GTest.hpp"

#include "triglav/io/File.hpp"

#include <print>

using namespace triglav::io::path_literals;

int main(int argc, char** argv)
{
   for (const auto [file_name, is_dir] : triglav::io::list_files("/home/ego"_path)) {
      std::println("{}", file_name);
   }

   testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
