#pragma once

#include <vector>
#include <string_view>

namespace triglav::io {

std::vector<char> read_whole_file(std::string_view name);

}