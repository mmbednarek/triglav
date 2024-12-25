#pragma once

#include <ranges>

namespace triglav {

constexpr auto Range = std::ranges::views::iota;
constexpr auto Enumerate = std::ranges::views::enumerate;
constexpr auto Values = std::ranges::views::values;

}// namespace triglav