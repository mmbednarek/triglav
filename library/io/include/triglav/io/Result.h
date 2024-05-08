#pragma once

#include <expected>
#include <span>
#include <string_view>

#include "triglav/Int.hpp"

namespace triglav::io {

enum class Status
{
   Success,
   BrokenPipe,
   StreamIsClosed,
   InvalidFile,
   BufferTooSmall,
};

template<typename T>
using Result = std::expected<T, Status>;

}// namespace triglav::io