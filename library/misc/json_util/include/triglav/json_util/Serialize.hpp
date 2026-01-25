#pragma once

#include "triglav/io/Stream.hpp"
#include "triglav/meta/Meta.hpp"

namespace triglav::json_util {

bool serialize(const meta::ClassRef& dst, io::IWriter& writer, bool pretty_print = false);

}
