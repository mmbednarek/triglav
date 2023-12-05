#include "InternalMesh.h"

#include "Parser.h"

#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>

namespace {

geometry::IndexedVertex parse_index(const std::string &index)
{
   using geometry::g_invalidIndex;
   using geometry::Index;
   using geometry::IndexedVertex;

   const auto it1       = index.find('/');
   const auto vertex_id = static_cast<Index>(std::stoi(index.substr(0, it1)));
   if (it1 == std::string::npos) {
      return IndexedVertex{vertex_id, g_invalidIndex, g_invalidIndex};
   }

   const auto it2 = index.find('/', it1 + 1);
   if (it2 == std::string::npos) {
      const auto uv_id = static_cast<Index>(std::stoi(index.substr(it1 + 1)));
      return IndexedVertex{vertex_id, uv_id, g_invalidIndex};
   }

   const auto uv_id     = static_cast<Index>(std::stoi(index.substr(it1 + 1, it2 - it1 - 1)));
   const auto normal_id = static_cast<Index>(std::stoi(index.substr(it2 + 1)));

   return {vertex_id, uv_id, normal_id};
}

glm::vec3 to_glm_vec3(const geometry::InternalMesh::Vector3 &vec)
{
   return glm::vec3{vec.x(), vec.y(), vec.z()};
}

glm::vec3 to_glm_vec3(const geometry::InternalMesh::Point3 &point)
{
   return glm::vec3{point.x(), point.y(), point.z()};
}

glm::vec2 to_glm_vec2(const geometry::InternalMesh::Vector2 &point)
{
   return glm::vec2{point.x(), point.y()};
}

}// namespace

namespace geometry {

InternalMesh::InternalMesh() :
    m_normalProperties(m_mesh.add_property_map<HalfedgeIndex, uint32_t>("h:normals", g_invalidIndex).first),
    m_uvProperties(m_mesh.add_property_map<HalfedgeIndex, uint32_t>("h:uvs", g_invalidIndex).first)
{
}

InternalMesh::VertexIndex InternalMesh::add_vertex(const Point3 location)
{
   return m_mesh.add_vertex(location);
}

InternalMesh::FaceIndex InternalMesh::add_face(const std::span<VertexIndex> vertices)
{
   return m_mesh.add_face(vertices);
}

void InternalMesh::triangulate_faces()
{
   if (this->is_triangulated())
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

   m_isTriangulated = true;
}

void InternalMesh::recalculate_normals() const
{
   // CGAL::Polygon_mesh_processing::compute_vertex_normals(m_mesh, m_normals);
}

InternalMesh::SurfaceMesh::Vertex_range InternalMesh::vertices() const
{
   return m_mesh.vertices();
}

InternalMesh::SurfaceMesh::Face_range InternalMesh::faces() const
{
   return m_mesh.faces();
}

InternalMesh::SurfaceMesh::Vertex_around_face_range InternalMesh::face_vertices(const FaceIndex index) const
{
   return m_mesh.vertices_around_face(m_mesh.halfedge(index));
}

InternalMesh::SurfaceMesh::Halfedge_around_face_range InternalMesh::face_halfedges(const FaceIndex index) const
{
   return m_mesh.halfedges_around_face(m_mesh.halfedge(index));
}

InternalMesh::VertexIndex InternalMesh::halfedge_target(const HalfedgeIndex index) const
{
   return m_mesh.target(index);
}

InternalMesh::Point3 InternalMesh::location(const VertexIndex index) const
{
   return m_mesh.point(index);
}

InternalMesh::Vector3 InternalMesh::normal(const HalfedgeIndex index)
{
   return m_normals[m_normalProperties[index]];
}

InternalMesh::Vector2 InternalMesh::uv(const HalfedgeIndex index)
{
   return m_uvs[m_uvProperties[index]];
}

InternalMesh::Vector3 InternalMesh::normal_by_id(const uint32_t index)
{
   if (index >= m_normals.size()) {
      return {0, 0, 1};
   }
   return m_normals[index];
}

InternalMesh::Vector2 InternalMesh::uv_by_id(const uint32_t index)
{
   if (index >= m_uvs.size()) {
      return {0, 0};
   }
   return m_uvs[index];
}

uint32_t InternalMesh::normal_id(const HalfedgeIndex index)
{
   return m_normalProperties[index];
}

uint32_t InternalMesh::uv_id(const HalfedgeIndex index)
{
   return m_uvProperties[index];
}

bool InternalMesh::is_triangulated()
{
   if (m_isTriangulated)
      return true;

   if (CGAL::is_triangle_mesh(m_mesh)) {
      m_isTriangulated = true;
      return true;
   }

   return false;
}

InternalMesh InternalMesh::from_obj_file(std::istream &stream)
{
   InternalMesh result;

   Parser parser(stream);
   parser.parse();

   for (const auto &[name, arguments] : parser.commands()) {
      if (name == "v") {
         assert(arguments.size() >= 3);
         result.add_vertex(
                 Point3{std::stof(arguments[0]), -std::stof(arguments[1]), std::stof(arguments[2])});
      } else if (name == "vn") {
         assert(arguments.size() >= 3);
         result.m_normals.emplace_back(std::stof(arguments[0]), -std::stof(arguments[1]),
                                       std::stof(arguments[2]));
      } else if (name == "vt") {
         assert(arguments.size() >= 2);
         result.m_uvs.emplace_back(std::stof(arguments[0]), 1 - std::stof(arguments[1]));
      } else if (name == "f") {
         std::vector<SurfaceMesh::vertex_index> vertexIds;
         std::vector<IndexedVertex> indices;

         for (const auto &attribute : arguments) {
            const auto index = parse_index(attribute);
            assert(index.uv <= result.m_uvs.size());
            assert(index.normal <= result.m_normals.size());

            indices.push_back(index);
            SurfaceMesh::vertex_index vertexId{index.location - 1};
            vertexIds.push_back(vertexId);
         }

         auto faceIndex = result.add_face(vertexIds);
         if (faceIndex.id() == g_invalidIndex)
            continue;

         int i = 0;
         for (const auto halfEdge : result.m_mesh.halfedges_around_face(result.m_mesh.halfedge(faceIndex))) {
            if (indices[i].normal != -1)
               result.m_normalProperties[halfEdge] = indices[i].normal - 1;

            if (indices[i].uv != -1)
               result.m_uvProperties[halfEdge] = indices[i].uv - 1;

            ++i;
         }
      }
   }

   return result;
}

InternalMesh InternalMesh::from_obj_file(const std::string_view path)
{
   std::ifstream stream(std::string(path), std::ios::binary);
   assert(stream.is_open());
   return InternalMesh::from_obj_file(stream);
}

graphics_api::Mesh<Vertex> InternalMesh::upload_to_device(graphics_api::Device &device)
{
   if (not this->is_triangulated())
      throw std::runtime_error("mesh must be triangulated before upload to GPU");

   std::map<IndexedVertex, uint32_t> indexMap;
   std::vector<Vertex> outVertices{};

   std::vector<uint32_t> outIndices{};
   for (const auto face_index : this->faces()) {
      for (const auto halfedge_index : this->face_halfedges(face_index)) {
         const auto vertex_index = this->halfedge_target(halfedge_index);
         const auto normal       = this->normal_id(halfedge_index);
         const auto uv           = this->uv_id(halfedge_index);
         IndexedVertex index{vertex_index.id(), uv, normal};

         if (indexMap.contains(index)) {
            outIndices.push_back(indexMap[index]);
            continue;
         }

         outVertices.push_back(Vertex(to_glm_vec3(this->location(vertex_index)),
                                      to_glm_vec2(this->uv_by_id(uv)),
                                      to_glm_vec3(this->normal_by_id(normal))));
         indexMap[index] = outVertices.size() - 1;
         outIndices.push_back(outVertices.size() - 1);
      }
   }

   graphics_api::VertexArray<Vertex> gpuVertices{device, outVertices.size()};
   gpuVertices.write(outVertices.data(), outVertices.size());

   graphics_api::IndexArray gpuIndices{device, outIndices.size()};
   gpuIndices.write(outIndices.data(), outIndices.size());

   return {std::move(gpuVertices), std::move(gpuIndices)};
}

}// namespace geometry
