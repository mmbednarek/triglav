#include "triglav/Int.hpp"
#include "triglav/io/File.hpp"
#include "triglav/io/Path.hpp"
#include "triglav/testing_core/GTest.hpp"

#include <fstream>
#include <ryml.hpp>
#include <tuple>
#include <vector>

namespace {

struct ResourcePath
{
   std::string name;
   std::string source;
};

std::vector<ResourcePath> parse_asset_list(const std::string_view path)
{
   std::vector<ResourcePath> result{};

   auto file = triglav::io::read_whole_file(triglav::io::Path{std::string{path}});
   auto tree = ryml::parse_in_place(c4::substr{const_cast<char*>(path.data()), path.size()}, c4::substr{file.data(), file.size()});
   auto resources = tree["resources"];

   for (const auto node : resources) {
      auto name = node["name"].val();
      auto source = node["source"].val();
      result.emplace_back(std::string{name.data(), name.size()}, std::string{source.data(), source.size()});
   }

   return result;
}

}// namespace

TEST(ParserTest, Base)
{
   auto list = parse_asset_list("../engine.yaml");
   ASSERT_FALSE(list.empty());
}
