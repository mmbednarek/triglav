#pragma once

#include "triglav/geometry/Mesh.hpp"

namespace triglav::gltf {

struct Document;
class BufferManager;

geometry::Mesh mesh_from_document(const Document& doc, u32 meshIndex, const BufferManager& bufferManager);
std::optional<geometry::Mesh> load_glb_mesh(const io::Path& path);

}// namespace triglav::gltf