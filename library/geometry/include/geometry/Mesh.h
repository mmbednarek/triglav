#pragma once

#include <memory>
#include <span>

#include "graphics_api/Array.hpp"

#include "Geometry.h"

namespace geometry {

class InternalMesh;

class Mesh
{
 public:
   Mesh();
   ~Mesh();

   Mesh(const Mesh &other)                = delete;
   Mesh &operator=(const Mesh &other)     = delete;
   Mesh(Mesh &&other) noexcept            = default;
   Mesh &operator=(Mesh &&other) noexcept = default;

   void recalculate_normals() const;
   void recalculate_tangents() const;
   void triangulate() const;
   void reverse_orientation();

   Index add_vertex(float x, float y, float z);
   Index add_face_range(std::span<Index> vertices);
   template<typename... TVertices>
   Index add_face(TVertices... vertices);
   Index add_uv(float u, float v);
   Index add_normal(float x, float y, float z);
   Index add_group(MeshGroup meshGroup);
   void set_face_uvs_range(Index face, std::span<Index> vertices);
   template<typename... TVertices>
   void set_face_uvs(Index face, TVertices... vertices);
   void set_face_normals_range(Index face, std::span<Index> vertices);
   template<typename... TVertices>
   void set_face_normals(Index face, TVertices... vertices);
   void set_face_group(Index face, Index group);
   void set_material(Index meshGroup, std::string_view material);

   [[nodiscard]] BoundingBox calculate_bouding_box() const;
   [[nodiscard]] bool is_triangulated() const;
   [[nodiscard]] size_t vertex_count() const;
   [[nodiscard]] DeviceMesh upload_to_device(graphics_api::Device &device) const;

   static Mesh from_file(std::string_view path);

 private:
   explicit Mesh(std::unique_ptr<InternalMesh> mesh);

   std::unique_ptr<InternalMesh> m_mesh;
};

template<typename... TVertices>
Index Mesh::add_face(TVertices... vertices)
{
   static_assert(sizeof...(vertices) >= 3);
   std::array<Index, sizeof...(vertices)> inVertices{static_cast<Index>(vertices)...};
   return this->add_face_range(inVertices);
}

template<typename... TVertices>
void Mesh::set_face_uvs(Index face, TVertices... vertices)
{
   static_assert(sizeof...(vertices) >= 3);
   std::array<Index, sizeof...(vertices)> inVertices{static_cast<Index>(vertices)...};
   this->set_face_uvs_range(face, inVertices);
}

template<typename... TVertices>
void Mesh::set_face_normals(Index face, TVertices... vertices)
{
   static_assert(sizeof...(vertices) >= 3);
   std::array<Index, sizeof...(vertices)> inVertices{static_cast<Index>(vertices)...};
   this->set_face_normals_range(face, inVertices);
}

}// namespace geometry