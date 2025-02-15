#pragma once

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Surface_mesh.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <optional>

#include "triglav/io/Path.hpp"
#include "triglav/io/Stream.hpp"

#include "Geometry.hpp"

namespace triglav::geometry {

constexpr auto g_NAN = std::numeric_limits<float>::quiet_NaN();

class InternalMesh
{
 public:
   using Kernel = CGAL::Simple_cartesian<float>;
   using Point3 = Kernel::Point_3;
   using Vector3 = Kernel::Vector_3;
   using SurfaceMesh = CGAL::Surface_mesh<Point3>;
   using VertexIndex = SurfaceMesh::vertex_index;
   using FaceIndex = SurfaceMesh::face_index;
   using HalfedgeIndex = SurfaceMesh::halfedge_index;

   struct Tangent
   {
      glm::vec3 vector{};
      float sign{};
   };

   using NormalPropMap = SurfaceMesh::Property_map<HalfedgeIndex, std::optional<glm::vec3>>;
   using TangentPropMap = SurfaceMesh::Property_map<HalfedgeIndex, std::optional<Tangent>>;
   using UvPropMap = SurfaceMesh::Property_map<HalfedgeIndex, std::optional<glm::vec2>>;
   using GroupPropMap = SurfaceMesh::Property_map<FaceIndex, Index>;

   InternalMesh();

   FaceIndex add_face(std::span<VertexIndex> vertices);
   VertexIndex add_vertex(glm::vec3 location);
   Index add_group(MeshGroup normal);
   [[nodiscard]] size_t vertex_count() const;
   void triangulate_faces();
   void recalculate_normals();
   void recalculate_tangents();
   [[nodiscard]] SurfaceMesh::Vertex_range vertices() const;
   [[nodiscard]] SurfaceMesh::Face_range faces() const;

   [[nodiscard]] SurfaceMesh::Vertex_around_face_range face_vertices(FaceIndex index) const;
   [[nodiscard]] SurfaceMesh::Halfedge_around_face_range face_halfedges(FaceIndex index) const;
   [[nodiscard]] VertexIndex halfedge_target(HalfedgeIndex index) const;

   void set_face_uvs(Index face, std::span<glm::vec2> uvs);
   void set_face_normals(Index face, std::span<glm::vec3> normals);
   void set_face_group(Index face, Index group);
   void set_material(Index meshGroup, std::string_view material);

   [[nodiscard]] glm::vec3 location(VertexIndex index) const;
   std::optional<glm::vec3> normal(HalfedgeIndex index) const;
   std::optional<glm::vec2> uv(HalfedgeIndex index) const;

   [[nodiscard]] bool is_triangulated();
   [[nodiscard]] BoundingBox calculate_bouding_box() const;

   [[nodiscard]] DeviceMesh upload_to_device(graphics_api::Device& device, graphics_api::BufferUsageFlags usageFlags);
   [[nodiscard]] VertexData to_vertex_data();
   void reverse_orientation();

   static InternalMesh from_obj_file(io::IReader& stream);
   static InternalMesh from_obj_file(const io::Path& path);

 private:
   SurfaceMesh m_mesh;
   NormalPropMap m_normals;
   UvPropMap m_uvs;
   GroupPropMap m_groupIds;
   TangentPropMap m_tangents;
   std::vector<MeshGroup> m_groups;
   bool m_isTriangulated{false};
};

}// namespace triglav::geometry