#pragma once

#include <memory>
#include <span>

#include "triglav/graphics_api/Array.hpp"
#include "triglav/io/Path.hpp"

#include "Geometry.hpp"

namespace triglav::geometry {

class InternalMesh;

class Mesh
{
 public:
   Mesh();
   ~Mesh();

   Mesh(const Mesh& other) = delete;
   Mesh& operator=(const Mesh& other) = delete;
   Mesh(Mesh&& other) noexcept;
   Mesh& operator=(Mesh&& other) noexcept;

   void recalculate_normals() const;
   void recalculate_tangents() const;
   void triangulate() const;
   void reverse_orientation();

   Index add_vertex(float x, float y, float z);
   Index add_vertex(Vector3 vertex);
   Index add_face_range(std::span<Index> vertices);
   template<typename... TVertices>
   Index add_face(TVertices... vertices);
   Index add_group(MeshGroup mesh_group);
   void set_face_uvs_range(Index face, std::span<glm::vec2> vertices);
   template<typename... TUVs>
   void set_face_uvs(Index face, TUVs... vertices);
   void set_face_normals_range(Index face, std::span<glm::vec3> vertices);
   template<typename... TNormals>
   void set_face_normals(Index face, TNormals... vertices);
   void set_face_group(Index face, Index group);
   void set_material(Index mesh_group, MaterialName material);

   [[nodiscard]] BoundingBox calculate_bounding_box() const;
   [[nodiscard]] bool is_triangulated() const;
   [[nodiscard]] size_t vertex_count() const;
   [[nodiscard]] DeviceMesh upload_to_device(graphics_api::Device& device,
                                             graphics_api::BufferUsageFlags usage_flags = graphics_api::BufferUsage::None) const;
   [[nodiscard]] VertexData to_vertex_data() const;

   static Mesh from_file(const io::Path& path);

 private:
   explicit Mesh(std::unique_ptr<InternalMesh> mesh);

   std::unique_ptr<InternalMesh> m_mesh;
};

template<typename... TVertices>
Index Mesh::add_face(TVertices... vertices)
{
   static_assert(sizeof...(vertices) >= 3);
   std::array<Index, sizeof...(vertices)> in_vertices{static_cast<Index>(vertices)...};
   return this->add_face_range(in_vertices);
}

template<typename... TUVs>
void Mesh::set_face_uvs(Index face, TUVs... vertices)
{
   static_assert(sizeof...(vertices) >= 3);
   std::array<glm::vec2, sizeof...(vertices)> in_vertices{vertices...};
   this->set_face_uvs_range(face, in_vertices);
}

template<typename... TNormals>
void Mesh::set_face_normals(Index face, TNormals... vertices)
{
   static_assert(sizeof...(vertices) >= 3);
   std::array<glm::vec3, sizeof...(vertices)> in_vertices{vertices...};
   this->set_face_normals_range(face, in_vertices);
}

}// namespace triglav::geometry