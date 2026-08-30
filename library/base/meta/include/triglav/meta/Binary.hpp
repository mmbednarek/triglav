#pragma once

#include "Meta.hpp"

#include "triglav/io/Stream.hpp"

namespace triglav::meta {

bool serialize_binary(io::IWriter& writer, const Ref& ref);
void deserialize_binary(io::IReader& reader, const Ref& ref);

}// namespace triglav::meta