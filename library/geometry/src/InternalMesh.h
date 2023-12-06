#pragma once

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Surface_mesh.h>

#include "../../graphics_api/include/graphics_api/Array.hpp"

#include "../include/geometry/Geometry.h"

namespace geometry {

class InternalMesh
{
public:
   using Kernel = CGAL::Simple_cartesian<float>;
   using Point3 = Kernel::Point_3;
   using Vector3 = Kernel::Vector_3;
   using Vector2 = Kernel::Vector_2;
   using SurfaceMesh = CGAL::Surface_mesh<Point3>;
   using VertexIndex = SurfaceMesh::vertex_index;
   using FaceIndex = SurfaceMesh::face_index;
   using HalfedgeIndex = SurfaceMesh::halfedge_index;
   using NormalPropMap = SurfaceMesh::Property_map<HalfedgeIndex, uint32_t>;
   using UvPropMap = SurfaceMesh::Property_map<HalfedgeIndex, uint32_t>;

   InternalMesh();

   VertexIndex add_vertex(Point3 location);
   FaceIndex add_face(std::span<VertexIndex> vertices);
   Index add_uv(Vector2 uv);
   Index add_normal(Vector3 normal);
   [[nodiscard]] size_t vertex_count() const;
   void triangulate_faces();
   void recalculate_normals();
   [[nodiscard]] SurfaceMesh::Vertex_range vertices() const;
   [[nodiscard]] SurfaceMesh::Face_range faces() const;

   [[nodiscard]] SurfaceMesh::Vertex_around_face_range face_vertices(FaceIndex index) const;
   [[nodiscard]] SurfaceMesh::Halfedge_around_face_range face_halfedges(FaceIndex index) const;
   [[nodiscard]] VertexIndex halfedge_target(HalfedgeIndex index) const;

   void set_face_uvs(Index face, std::span<Index> uvs);
   void set_face_normals(Index face, std::span<Index> normals);

   [[nodiscard]] Point3 location(VertexIndex index) const;
   Vector3 normal(HalfedgeIndex index);
   Vector2 uv(HalfedgeIndex index);

   Vector3 normal_by_id(uint32_t index);
   Vector2 uv_by_id(uint32_t index);

   uint32_t normal_id(HalfedgeIndex index);
   uint32_t uv_id(HalfedgeIndex index);

   [[nodiscard]] bool is_triangulated();

   graphics_api::Mesh<Vertex> upload_to_device(graphics_api::Device &device);
   void reverse_orientation();

   static InternalMesh from_obj_file(std::istream& stream);
   static InternalMesh from_obj_file(std::string_view path);

private:
   SurfaceMesh m_mesh;
   NormalPropMap m_normalProperties;
   UvPropMap m_uvProperties;
   std::vector<Vector3> m_normals;
   std::vector<Vector2> m_uvs;
   bool m_isTriangulated{false};
};

}