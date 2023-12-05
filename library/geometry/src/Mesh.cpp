#include "Mesh.h"

#include "Parser.h"

#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <iostream>

namespace {

struct Index
{
   int vertex_id, uv_id, normal_id;
   auto operator<=>(const Index &rhs) const = default;
};

Index parse_index(const std::string &index)
{
   const auto it1       = index.find('/');
   const auto vertex_id = std::stoi(index.substr(0, it1));
   if (it1 == std::string::npos) {
      return Index{vertex_id, -1, -1};
   }

   const auto it2 = index.find('/', it1 + 1);
   if (it2 == std::string::npos) {
      const auto uv_id = std::stoi(index.substr(it1 + 1));
      return Index{vertex_id, uv_id, -1};
   }

   const auto uv_id     = std::stoi(index.substr(it1 + 1, it2 - it1 - 1));
   const auto normal_id = std::stoi(index.substr(it2 + 1));

   return Index(vertex_id, uv_id, normal_id);
}

}// namespace

namespace object_reader {

constexpr auto g_invalidIndex = std::numeric_limits<uint32_t>::max();

Mesh::Mesh() :
    m_normalProperties(m_mesh.add_property_map<HalfedgeIndex, uint32_t>("h:normals", g_invalidIndex).first),
    m_uvProperties(m_mesh.add_property_map<HalfedgeIndex, uint32_t>("h:uvs", g_invalidIndex).first)
{
}

Mesh::VertexIndex Mesh::add_vertex(const Point3 location)
{
   return m_mesh.add_vertex(location);
}

Mesh::FaceIndex Mesh::add_face(const std::span<VertexIndex> vertices)
{
   return m_mesh.add_face(vertices);
}

void Mesh::triangulate_faces()
{
   if (CGAL::is_triangle_mesh(m_mesh))
      return;

   CGAL::Polygon_mesh_processing::triangulate_faces(m_mesh);

   for (const auto halfedge : m_mesh.halfedges()) {
      if (m_normalProperties[halfedge] != g_invalidIndex && m_uvProperties[halfedge] != g_invalidIndex)
         continue;

      HalfedgeIndex corresponding_halfedge = m_mesh.prev_around_target(halfedge);
      while (corresponding_halfedge != halfedge &&
             m_normalProperties[corresponding_halfedge] == g_invalidIndex &&
             m_uvProperties[corresponding_halfedge] == g_invalidIndex) {
         corresponding_halfedge = m_mesh.prev_around_target(corresponding_halfedge);
      }

      if (m_normalProperties[corresponding_halfedge] == g_invalidIndex)
         continue;

      m_normalProperties[halfedge] = m_normalProperties[corresponding_halfedge];
      m_uvProperties[halfedge]     = m_uvProperties[corresponding_halfedge];
   }
}

void Mesh::recalculate_normals() const
{
   // CGAL::Polygon_mesh_processing::compute_vertex_normals(m_mesh, m_normals);
}

Mesh::SurfaceMesh::Vertex_range Mesh::vertices() const
{
   return m_mesh.vertices();
}

Mesh::SurfaceMesh::Face_range Mesh::faces() const
{
   return m_mesh.faces();
}

void Mesh::debug_stats()
{
   // std::cout << "verts: " << m_mesh.vertices().size() << ", faces: " << m_mesh.faces().size() << ", halfedges: " << m_mesh.halfedges().size() << '\n';

   // for (const auto face : this->faces()) {
   //    for (const auto he : m_mesh.halfedges_around_face(m_mesh.halfedge(face))) {
   //       std::cout << std::format("{} ", m_uvProperties[he]);
   //    }
   //
   //    std::cout << '\n';
   // }

   for (const auto vertex : this->vertices()) {
      std::cout << vertex.id() << ": ";
      for (const auto he : m_mesh.halfedges_around_target(m_mesh.halfedge(vertex))) {
         auto reverse = m_mesh.opposite(he);
         HalfedgeIndex hbb;
         for (const auto hb : m_mesh.halfedges_around_face(reverse)) {
            if (m_mesh.target(he) == m_mesh.target(hb)) {
               hbb = hb;
               break;
            }
         }
         // m_mesh.halfedge();
         std::cout << std::format("{} {} (face: {}) ", m_uvProperties[he], m_uvProperties[hbb],
                                  m_mesh.face(he).id());
      }
      std::cout << '\n';
   }
}

Mesh::SurfaceMesh::Vertex_around_face_range Mesh::face_vertices(const FaceIndex index) const
{
   return m_mesh.vertices_around_face(m_mesh.halfedge(index));
}

Mesh::SurfaceMesh::Halfedge_around_face_range Mesh::face_halfedges(const FaceIndex index) const
{
   return m_mesh.halfedges_around_face(m_mesh.halfedge(index));
}

Mesh::VertexIndex Mesh::halfedge_target(const HalfedgeIndex index) const
{
   return m_mesh.target(index);
}

Mesh::Point3 Mesh::location(const VertexIndex index) const
{
   return m_mesh.point(index);
}

Mesh::Vector3 Mesh::normal(const HalfedgeIndex index)
{
   return m_normals[m_normalProperties[index]];
}

Mesh::Vector2 Mesh::uv(const HalfedgeIndex index)
{
   return m_uvs[m_uvProperties[index]];
}

Mesh::Vector3 Mesh::normal_by_id(const uint32_t index)
{
   if (index >= m_normals.size()) {
      return {0, 0, 1};
   }
   return m_normals[index];
}

Mesh::Vector2 Mesh::uv_by_id(const uint32_t index)
{
   if (index >= m_uvs.size()) {
      return {0, 0};
   }
   return m_uvs[index];
}

uint32_t Mesh::normal_id(const HalfedgeIndex index)
{
   return m_normalProperties[index];
}

uint32_t Mesh::uv_id(const HalfedgeIndex index)
{
   return m_uvProperties[index];
}

Mesh Mesh::from_obj_file(std::istream &stream)
{
   Mesh result;

   Parser parser(stream);
   parser.parse();

   for (const auto &[name, arguments] : parser.commands()) {
      if (name == "v") {
         assert(arguments.size() >= 3);
         result.add_vertex(Point3{std::stof(arguments[0]), -std::stof(arguments[1]), std::stof(arguments[2])});
      } else if (name == "vn") {
         assert(arguments.size() >= 3);
         result.m_normals.emplace_back(std::stof(arguments[0]), -std::stof(arguments[1]),
                                       std::stof(arguments[2]));
      } else if (name == "vt") {
         assert(arguments.size() >= 2);
         result.m_uvs.emplace_back(std::stof(arguments[0]), 1 - std::stof(arguments[1]));
      } else if (name == "f") {
         std::vector<SurfaceMesh::vertex_index> vertexIds;
         std::vector<Index> indices;

         for (const auto &attribute : arguments) {
            const auto index = parse_index(attribute);
            assert(index.uv_id <= result.m_uvs.size());
            assert(index.normal_id <= result.m_normals.size());

            indices.push_back(index);
            SurfaceMesh::vertex_index vertexId{static_cast<uint32_t>(index.vertex_id - 1)};
            vertexIds.push_back(vertexId);
         }

         auto faceIndex = result.add_face(vertexIds);
         if (faceIndex.id() == g_invalidIndex)
            continue;

         int i = 0;
         for (const auto halfEdge : result.m_mesh.halfedges_around_face(result.m_mesh.halfedge(faceIndex))) {
            if (indices[i].normal_id != -1)
               result.m_normalProperties[halfEdge] = indices[i].normal_id - 1;

            if (indices[i].uv_id != -1)
               result.m_uvProperties[halfEdge] = indices[i].uv_id - 1;

            ++i;
         }
      }
   }

   return result;
}

Mesh Mesh::from_obj_file(const std::string_view path)
{
   std::ifstream stream(std::string(path), std::ios::binary);
   assert(stream.is_open());
   return Mesh::from_obj_file(stream);
}

}// namespace object_reader