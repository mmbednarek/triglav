#pragma once

#include "Meta.hpp"

#include "triglav/io/Stream.hpp"

namespace triglav::meta {

void display(const Ref& ref, io::IWriter& writer);

}// namespace triglav::meta