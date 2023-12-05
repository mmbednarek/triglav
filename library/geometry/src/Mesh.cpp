#include "Mesh.h"
#include "InternalMesh.h"

namespace geometry {

bool Mesh::is_triangulated() const
{
   assert(m_mesh != nullptr);
   return m_mesh->is_triangulated();
}

void Mesh::triangulate() const
{
   assert(m_mesh != nullptr);
   m_mesh->triangulate_faces();
}

graphics_api::Mesh<Vertex> Mesh::upload_to_device(graphics_api::Device &device) const
{
   assert(m_mesh != nullptr);
   return m_mesh->upload_to_device(device);
}

Mesh Mesh::from_file(const std::string_view path)
{
   auto internalMesh = InternalMesh::from_obj_file(path);
   return Mesh(std::make_unique<InternalMesh>(std::move(internalMesh)));
}

Mesh::Mesh(std::unique_ptr<InternalMesh> mesh) :
   m_mesh(std::move(mesh))
{
}


}// namespace geometry