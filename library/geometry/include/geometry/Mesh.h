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

   [[nodiscard]] bool is_triangulated() const;
   void recalculate_normals() const;
   void triangulate() const;
   Index add_vertex(float x, float y, float z);
   Index add_face_range(std::span<Index> vertices);
   Index add_uv(float u, float v);
   Index add_normal(float x, float y, float z);
   size_t vertex_count();
   void set_face_uvs_range(Index face, std::span<Index> vertices);
   void set_face_normals_range(Index face, std::span<Index> vertices);
   void reverse_orientation();
   graphics_api::Mesh<Vertex> upload_to_device(graphics_api::Device &device) const;

   template<typename... TVertices>
   Index add_face(TVertices... vertices)
   {
      static_assert(sizeof...(vertices) >= 3);
      std::array<Index, sizeof...(vertices)> inVertices{static_cast<Index>(vertices)...};
      return this->add_face_range(inVertices);
   }

   template<typename... TVertices>
   void set_face_uvs(Index face, TVertices... vertices)
   {
      static_assert(sizeof...(vertices) >= 3);
      std::array<Index, sizeof...(vertices)> inVertices{static_cast<Index>(vertices)...};
      this->set_face_uvs_range(face, inVertices);
   }

   template<typename... TVertices>
   void set_face_normals(Index face, TVertices... vertices)
   {
      static_assert(sizeof...(vertices) >= 3);
      std::array<Index, sizeof...(vertices)> inVertices{static_cast<Index>(vertices)...};
      this->set_face_normals_range(face, inVertices);
   }

   static Mesh from_file(std::string_view path);

 private:
   explicit Mesh(std::unique_ptr<InternalMesh> mesh);

   std::unique_ptr<InternalMesh> m_mesh;
};

}// namespace geometry