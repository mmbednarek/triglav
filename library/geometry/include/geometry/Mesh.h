#pragma once

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Surface_mesh.h>

namespace object_reader {

class Mesh
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

   Mesh();

   VertexIndex add_vertex(Point3 location);
   FaceIndex add_face(std::span<VertexIndex> vertices);
   void triangulate_faces();
   void recalculate_normals() const;
   [[nodiscard]] SurfaceMesh::Vertex_range vertices() const;
   [[nodiscard]] SurfaceMesh::Face_range faces() const;
   void debug_stats();

   [[nodiscard]] SurfaceMesh::Vertex_around_face_range face_vertices(FaceIndex index) const;
   [[nodiscard]] SurfaceMesh::Halfedge_around_face_range face_halfedges(FaceIndex index) const;
   [[nodiscard]] VertexIndex halfedge_target(HalfedgeIndex index) const;

   [[nodiscard]] Point3 location(VertexIndex index) const;
   Vector3 normal(HalfedgeIndex index);
   Vector2 uv(HalfedgeIndex index);

   Vector3 normal_by_id(uint32_t index);
   Vector2 uv_by_id(uint32_t index);

   uint32_t normal_id(HalfedgeIndex index);
   uint32_t uv_id(HalfedgeIndex index);

   static Mesh from_obj_file(std::istream& stream);
   static Mesh from_obj_file(std::string_view path);

private:
   SurfaceMesh m_mesh;
   NormalPropMap m_normalProperties;
   UvPropMap m_uvProperties;
   std::vector<Vector3> m_normals;
   std::vector<Vector2> m_uvs;
};

}