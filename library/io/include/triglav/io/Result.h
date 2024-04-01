#pragma once

#include <string_view>
#include <span>
#include <expected>

#include "triglav/Int.hpp"

namespace triglav::io {

enum class Status {
    Success,
    BrokenPipe,
    StreamIsClosed,
   InvalidFile,
};

template<typename T>
using Result = std::expected<T, Status>;

}