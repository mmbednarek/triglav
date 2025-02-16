#pragma once

#include "triglav/geometry/Mesh.hpp"
#include "triglav/io/Stream.hpp"

namespace triglav::asset {

bool encode_mesh(const geometry::Mesh& mesh, io::IWriter& writer);

}// namespace triglav::asset