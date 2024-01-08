#include "Mesh.h"
#include "InternalMesh.h"

namespace geometry {

Mesh::Mesh() :
    m_mesh(std::make_unique<InternalMesh>())
{
}

Mesh::~Mesh() = default;

bool Mesh::is_triangulated() const
{
   assert(m_mesh != nullptr);
   return m_mesh->is_triangulated();
}

void Mesh::recalculate_normals() const
{
   assert(m_mesh != nullptr);
   return m_mesh->recalculate_normals();
}

void Mesh::recalculate_tangents() const
{
   assert(m_mesh != nullptr);
   return m_mesh->recalculate_tangents();
}

void Mesh::triangulate() const
{
   assert(m_mesh != nullptr);
   m_mesh->triangulate_faces();
}

// ReSharper disable once CppMemberFunctionMayBeConst
Index Mesh::add_vertex(float x, float y, float z)
{
   assert(m_mesh != nullptr);
   return m_mesh->add_vertex({x, y, z}).id();
}

// ReSharper disable once CppMemberFunctionMayBeConst
Index Mesh::add_face_range(std::span<Index> vertices)
{
   std::vector<InternalMesh::VertexIndex> vecVertices{};
   vecVertices.resize(vertices.size());
   std::ranges::transform(vertices, vecVertices.begin(),
                          [](const Index i) { return InternalMesh::VertexIndex{i}; });
   return m_mesh->add_face(vecVertices);
}

// ReSharper disable once CppMemberFunctionMayBeConst
Index Mesh::add_uv(float u, float v)
{
   assert(m_mesh != nullptr);
   return m_mesh->add_uv({u, v});
}

// ReSharper disable once CppMemberFunctionMayBeConst
Index Mesh::add_normal(float x, float y, float z)
{
   assert(m_mesh != nullptr);
   return m_mesh->add_normal({x, y, z});
}

// ReSharper disable once CppMemberFunctionMayBeConst
Index Mesh::add_group(MeshGroup meshGroup)
{
   assert(m_mesh != nullptr);
   return m_mesh->add_group(std::move(meshGroup));
}

size_t Mesh::vertex_count() const
{
   assert(m_mesh != nullptr);
   return m_mesh->vertex_count();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Mesh::set_face_uvs_range(const Index face, const std::span<Index> vertices)
{
   assert(m_mesh != nullptr);
   return m_mesh->set_face_uvs(face, vertices);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Mesh::set_face_normals_range(const Index face, const std::span<Index> vertices)
{
   assert(m_mesh != nullptr);
   return m_mesh->set_face_normals(face, vertices);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Mesh::set_face_group(const Index face, const Index group)
{
   assert(m_mesh != nullptr);
   return m_mesh->set_face_group(face, group);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Mesh::set_material(const Index meshGroup, const std::string_view material)
{
   assert(m_mesh != nullptr);
   m_mesh->set_material(meshGroup, material);
}

BoundingBox Mesh::calculate_bouding_box() const
{
   assert(m_mesh != nullptr);
   return m_mesh->calculate_bouding_box();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Mesh::reverse_orientation()
{
   assert(m_mesh != nullptr);
   m_mesh->reverse_orientation();
}

DeviceMesh Mesh::upload_to_device(graphics_api::Device &device) const
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