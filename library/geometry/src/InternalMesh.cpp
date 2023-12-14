#include "InternalMesh.h"

#include "Parser.h"

#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
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
    m_uvProperties(m_mesh.add_property_map<HalfedgeIndex, uint32_t>("h:uvs", g_invalidIndex).first),
    m_groupProperties(m_mesh.add_property_map<FaceIndex, Index>("f:groups", g_invalidIndex).first)
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

Index InternalMesh::add_uv(Vector2 uv)
{
   m_uvs.emplace_back(uv);
   return m_uvs.size() - 1;
}

Index InternalMesh::add_normal(Vector3 normal)
{
   m_normals.emplace_back(normal);
   return m_normals.size() - 1;
}

Index InternalMesh::add_group(MeshGroup normal)
{
   m_groups.emplace_back(std::move(normal));
   return m_groups.size() - 1;
}

size_t InternalMesh::vertex_count() const
{
   return m_mesh.vertices().size();
}

void InternalMesh::triangulate_faces()
{
   if (this->is_triangulated())
      return;

   CGAL::Polygon_mesh_processing::triangulate_faces(m_mesh);

   // Fix group properties for faces.
   bool allFacesFixed = false;
   int limit = 3;
   while (not allFacesFixed) {
      if (limit == 0)
         break;
      --limit;

      allFacesFixed = true;
      for (const auto face : m_mesh.faces()) {
         if (m_groupProperties[face] != g_invalidIndex)
            continue;

         auto origHalfedge = m_mesh.halfedge(face);
         auto halfedge = m_mesh.prev_around_target(origHalfedge);
         auto sourceFace = m_mesh.face(halfedge);
         while (halfedge != origHalfedge && m_groupProperties[sourceFace] == g_invalidIndex) {
            halfedge = m_mesh.prev_around_target(halfedge);
            sourceFace = m_mesh.face(halfedge );
         }

         if (halfedge == origHalfedge) {
            allFacesFixed = false;
            continue;
         }

         m_groupProperties[face] = m_groupProperties[sourceFace];
      }
   }

   // Fix normals and uvs for halfedges.
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

void InternalMesh::recalculate_normals()
{
   const auto vertexNormals =
           m_mesh.add_property_map<VertexIndex, Vector3>("v:normals", Vector3(0, 0, 0)).first;
   CGAL::Polygon_mesh_processing::compute_vertex_normals(m_mesh, vertexNormals);
   m_normals.resize(m_mesh.vertices().size());

   for (const auto vertex : m_mesh.vertices()) {
      m_normals[vertex.id()] = vertexNormals[vertex];
      for (const auto halfedge : m_mesh.halfedges_around_target(m_mesh.halfedge(vertex))) {
         m_normalProperties[halfedge] = vertex.id();
      }
   }
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

InternalMesh::SurfaceMesh::Halfedge_around_face_range
InternalMesh::face_halfedges(const FaceIndex index) const
{
   return m_mesh.halfedges_around_face(m_mesh.halfedge(index));
}

InternalMesh::VertexIndex InternalMesh::halfedge_target(const HalfedgeIndex index) const
{
   return m_mesh.target(index);
}

void InternalMesh::set_face_uvs(const Index face, std::span<Index> uvs)
{
   auto it = uvs.begin();
   for (const auto halfedge : m_mesh.halfedges_around_face(m_mesh.halfedge(FaceIndex{face}))) {
      if (it == uvs.end())
         break;

      m_uvProperties[halfedge] = *it;

      ++it;
   }
}

void InternalMesh::set_face_normals(const Index face, std::span<Index> normals)
{
   auto it = normals.begin();
   for (const auto halfedge : m_mesh.halfedges_around_face(m_mesh.halfedge(FaceIndex{face}))) {
      if (it == normals.end())
         break;

      m_normalProperties[halfedge] = *it;

      ++it;
   }
}

void InternalMesh::set_face_group(const Index face, const Index group)
{
   m_groupProperties[FaceIndex{face}] = group;
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

   Index lastGroupIndex = g_invalidIndex;

   for (const auto &[name, arguments] : parser.commands()) {
      if (name == "v") {
         assert(arguments.size() >= 3);
         result.add_vertex(
                 Point3{std::stof(arguments[0]), -std::stof(arguments[1]), std::stof(arguments[2])});
      } else if (name == "vn") {
         assert(arguments.size() >= 3);
         result.m_normals.emplace_back(std::stof(arguments[0]), std::stof(arguments[1]),
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

         result.m_groupProperties[faceIndex] = lastGroupIndex;

         int i = 0;
         for (const auto halfEdge : result.m_mesh.halfedges_around_face(result.m_mesh.halfedge(faceIndex))) {
            if (indices[i].normal != -1)
               result.m_normalProperties[halfEdge] = indices[i].normal - 1;

            if (indices[i].uv != -1)
               result.m_uvProperties[halfEdge] = indices[i].uv - 1;

            ++i;
         }
      } else if (name == "o") {
         assert(arguments.size() == 1);
         lastGroupIndex = result.add_group({arguments[0], ""});
      } else if (name == "usemtl") {
         assert(arguments.size() == 1);
         if (not result.m_groups.empty()) {
            result.m_groups[lastGroupIndex].material = arguments[0];
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

DeviceMesh InternalMesh::upload_to_device(graphics_api::Device &device)
{
   if (not this->is_triangulated())
      throw std::runtime_error("mesh must be triangulated before upload to GPU");
   assert(m_mesh.faces().size() > 0);

   std::map<IndexedVertex, uint32_t> indexMap;
   std::vector<Vertex> outVertices{};

   std::vector<MaterialRange> materialRanges{};
   std::string currentMaterial;

   size_t lastOffset{};

   std::vector<uint32_t> outIndices{};
   for (const auto face_index : this->faces()) {
      const auto groupId = m_groupProperties[face_index];
      if (groupId != g_invalidIndex) {
         const auto& group = m_groups[groupId];
         if (group.material != currentMaterial) {
            if (lastOffset != outIndices.size()) {
               materialRanges.push_back(MaterialRange{lastOffset, outIndices.size() - lastOffset, currentMaterial});
            }
            currentMaterial = group.material;
            lastOffset = outIndices.size();
         }
      }

      for (const auto halfedge_index : this->face_halfedges(face_index)) {
         const auto vertex_index = this->halfedge_target(halfedge_index);
         const auto normal       = this->normal_id(halfedge_index);
         const auto uv           = this->uv_id(halfedge_index);

         IndexedVertex index{vertex_index.id(), uv, normal};

         if (indexMap.contains(index)) {
            outIndices.push_back(indexMap[index]);
         } else {
            outVertices.push_back(Vertex(to_glm_vec3(this->location(vertex_index)),
                                         to_glm_vec2(this->uv_by_id(uv)),
                                         to_glm_vec3(this->normal_by_id(normal))));
            indexMap[index] = outVertices.size() - 1;
            outIndices.push_back(outVertices.size() - 1);
         }
      }
   }

   if (lastOffset != outIndices.size()) {
      materialRanges.push_back(MaterialRange{lastOffset, outIndices.size() - lastOffset, currentMaterial});
   }

   graphics_api::VertexArray<Vertex> gpuVertices{device, outVertices.size()};
   gpuVertices.write(outVertices.data(), outVertices.size());

   graphics_api::IndexArray gpuIndices{device, outIndices.size()};
   gpuIndices.write(outIndices.data(), outIndices.size());

   return {{std::move(gpuVertices), std::move(gpuIndices)}, std::move(materialRanges)};
}

void InternalMesh::reverse_orientation()
{
   CGAL::Polygon_mesh_processing::reverse_face_orientations(m_mesh);
   for (auto &normal : m_normals) {
      normal = -normal;
   }
}

}// namespace geometry
