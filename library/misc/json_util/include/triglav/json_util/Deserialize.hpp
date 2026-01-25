#pragma once

#include "triglav/io/Stream.hpp"
#include "triglav/meta/Meta.hpp"

namespace triglav::json_util {

bool deserialize(const meta::ClassRef& dst, io::IReader& reader);

}