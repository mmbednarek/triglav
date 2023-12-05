#pragma once

#include <memory>

#include "graphics_api/Array.hpp"

#include "Geometry.h"


namespace geometry {

class InternalMesh;

class Mesh
{
public:
   [[nodiscard]] bool is_triangulated() const;
   void triangulate() const;
   graphics_api::Mesh<Vertex> upload_to_device(graphics_api::Device &device) const;

   static Mesh from_file(std::string_view path);

private:
   explicit Mesh(std::unique_ptr<InternalMesh> mesh);

   std::unique_ptr<InternalMesh> m_mesh;
};

}