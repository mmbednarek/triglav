#include "Mesh.hpp"
#include "InternalMesh.hpp"

namespace triglav::geometry {

Mesh::Mesh() :
    m_mesh(std::make_unique<InternalMesh>())
{
}

Mesh::~Mesh() = default;

Mesh::Mesh(Mesh&& other) noexcept = default;
Mesh& Mesh::operator=(Mesh&& other) noexcept = default;

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
Index Mesh::add_vertex(const Vector3 vertex)
{
   assert(m_mesh != nullptr);
   return m_mesh->add_vertex(vertex).id();
}

// ReSharper disable once CppMemberFunctionMayBeConst
Index Mesh::add_face_range(std::span<Index> vertices)
{
   std::vector<InternalMesh::VertexIndex> vec_vertices{};
   vec_vertices.resize(vertices.size());
   std::ranges::transform(vertices, vec_vertices.begin(), [](const Index i) { return InternalMesh::VertexIndex{i}; });
   return m_mesh->add_face(vec_vertices);
}

// ReSharper disable once CppMemberFunctionMayBeConst
Index Mesh::add_group(MeshGroup mesh_group)
{
   assert(m_mesh != nullptr);
   return m_mesh->add_group(std::move(mesh_group));
}

size_t Mesh::vertex_count() const
{
   assert(m_mesh != nullptr);
   return m_mesh->vertex_count();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Mesh::set_face_uvs_range(const Index face, const std::span<glm::vec2> vertices)
{
   assert(m_mesh != nullptr);
   return m_mesh->set_face_uvs(face, vertices);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Mesh::set_face_normals_range(const Index face, const std::span<glm::vec3> vertices)
{
   assert(m_mesh != nullptr);
   return m_mesh->set_face_normals(face, vertices);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Mesh::set_face_tangents_range(const Index face, const std::span<Vector4> tangents)
{
   assert(m_mesh != nullptr);
   return m_mesh->set_face_tangents(face, tangents);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Mesh::set_face_group(const Index face, const Index group)
{
   assert(m_mesh != nullptr);
   return m_mesh->set_face_group(face, group);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Mesh::set_material(const Index mesh_group, const MaterialName material)
{
   assert(m_mesh != nullptr);
   m_mesh->set_material(mesh_group, material);
}

BoundingBox Mesh::calculate_bounding_box() const
{
   assert(m_mesh != nullptr);
   return m_mesh->calculate_bounding_box();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Mesh::reverse_orientation()
{
   assert(m_mesh != nullptr);
   m_mesh->reverse_orientation();
}

DeviceMesh Mesh::upload_to_device(graphics_api::Device& device, const graphics_api::BufferUsageFlags usage_flags) const
{
   assert(m_mesh != nullptr);
   return m_mesh->upload_to_device(device, usage_flags);
}

VertexData Mesh::to_vertex_data() const
{
   assert(m_mesh != nullptr);
   return m_mesh->to_vertex_data();
}

Mesh Mesh::from_file(const io::Path& path)
{
   auto internal_mesh = InternalMesh::from_obj_file(path);
   return Mesh(std::make_unique<InternalMesh>(std::move(internal_mesh)));
}

Mesh::Mesh(std::unique_ptr<InternalMesh> mesh) :
    m_mesh(std::move(mesh))
{
}

}// namespace triglav::geometry