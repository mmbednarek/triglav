#pragma once

#include "triglav/geometry/Mesh.hpp"
#include "triglav/io/Stream.hpp"

#include <optional>

namespace triglav::asset {

bool encode_mesh(const geometry::Mesh& mesh, io::IWriter& writer);
std::optional<geometry::MeshData> decode_mesh(io::IReader& reader);

}// namespace triglav::asset