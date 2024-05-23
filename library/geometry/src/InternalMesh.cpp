#include "InternalMesh.h"

#include "Parser.h"

#include "triglav/io/File.h"

#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <glm/geometric.hpp>
#include <mikktspace/mikktspace.h>
#include <unordered_map>

namespace {

triglav::geometry::IndexedVertex parse_index(const std::string &index)
{
   using triglav::geometry::g_invalidIndex;
   using triglav::geometry::Index;
   using triglav::geometry::IndexedVertex;

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

glm::vec3 to_glm_vec3(const triglav::geometry::InternalMesh::Point3 &point)
{
   return glm::vec3{point.x(), point.y(), point.z()};
}

}// namespace

namespace triglav::geometry {

InternalMesh::InternalMesh() :
    m_normals(m_mesh.add_property_map<HalfedgeIndex, std::optional<glm::vec3>>("h:normals", std::nullopt)
                      .first),
    m_uvs(m_mesh.add_property_map<HalfedgeIndex, std::optional<glm::vec2>>("h:uvs", std::nullopt).first),
    m_groupIds(m_mesh.add_property_map<FaceIndex, Index>("f:groups", g_invalidIndex).first),
    m_tangents(
            m_mesh.add_property_map<HalfedgeIndex, std::optional<Tangent>>("h:tangents", std::nullopt).first)
{
}

InternalMesh::VertexIndex InternalMesh::add_vertex(const glm::vec3 location)
{
   return m_mesh.add_vertex(Point3{location.x, location.y, location.z});
}

InternalMesh::FaceIndex InternalMesh::add_face(const std::span<VertexIndex> vertices)
{
   return m_mesh.add_face(vertices);
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
   int limit          = 3;
   while (not allFacesFixed) {
      if (limit == 0)
         break;
      --limit;

      allFacesFixed = true;
      for (const auto face : m_mesh.faces()) {
         if (m_groupIds[face] != g_invalidIndex)
            continue;

         auto origHalfedge = m_mesh.halfedge(face);
         auto halfedge     = m_mesh.prev_around_target(origHalfedge);
         auto sourceFace   = m_mesh.face(halfedge);
         while (halfedge != origHalfedge && m_groupIds[sourceFace] == g_invalidIndex) {
            halfedge   = m_mesh.prev_around_target(halfedge);
            sourceFace = m_mesh.face(halfedge);
         }

         if (halfedge == origHalfedge) {
            allFacesFixed = false;
            continue;
         }

         m_groupIds[face] = m_groupIds[sourceFace];
      }
   }

   assert(allFacesFixed);

   // Fix normals and uvs for halfedges.
   for (const auto halfedge : m_mesh.halfedges()) {
      if (m_normals[halfedge].has_value() && m_uvs[halfedge].has_value())
         continue;

      HalfedgeIndex corresponding_halfedge = m_mesh.prev_around_target(halfedge);
      while (corresponding_halfedge != halfedge && not m_normals[corresponding_halfedge].has_value() &&
             not m_uvs[corresponding_halfedge].has_value()) {
         corresponding_halfedge = m_mesh.prev_around_target(corresponding_halfedge);
      }

      if (not m_normals[corresponding_halfedge].has_value())
         continue;

      m_normals[halfedge] = m_normals[corresponding_halfedge];
      m_uvs[halfedge]     = m_uvs[corresponding_halfedge];
   }

   m_isTriangulated = true;
}

void InternalMesh::recalculate_normals()
{
   const auto vertexNormals =
           m_mesh.add_property_map<VertexIndex, Vector3>("v:normals", Vector3(0, 0, 0)).first;
   CGAL::Polygon_mesh_processing::compute_vertex_normals(m_mesh, vertexNormals);

   for (const auto vertex : m_mesh.vertices()) {
      const auto normal = vertexNormals[vertex];
      for (const auto halfedge : m_mesh.halfedges_around_target(m_mesh.halfedge(vertex))) {
         m_normals[halfedge] = glm::vec3{normal.x(), normal.y(), normal.z()};
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

void InternalMesh::set_face_uvs(const Index face, std::span<glm::vec2> uvs)
{
   auto it = uvs.begin();
   for (const auto halfedge : m_mesh.halfedges_around_face(m_mesh.halfedge(FaceIndex{face}))) {
      if (it == uvs.end())
         break;

      m_uvs[halfedge] = *it;

      ++it;
   }
}

void InternalMesh::set_face_normals(const Index face, std::span<glm::vec3> normals)
{
   auto it = normals.begin();
   for (const auto halfedge : m_mesh.halfedges_around_face(m_mesh.halfedge(FaceIndex{face}))) {
      if (it == normals.end())
         break;

      m_normals[halfedge] = *it;

      ++it;
   }
}

void InternalMesh::set_face_group(const Index face, const Index group)
{
   m_groupIds[FaceIndex{face}] = group;
}

void InternalMesh::set_material(const Index meshGroup, const std::string_view material)
{
   m_groups[meshGroup].material = material;
}

glm::vec3 InternalMesh::location(const VertexIndex index) const
{
   const auto result = m_mesh.point(index);
   return {result.x(), result.y(), result.z()};
}

std::optional<glm::vec3> InternalMesh::normal(const HalfedgeIndex index) const
{
   return m_normals[index];
}

std::optional<glm::vec2> InternalMesh::uv(const HalfedgeIndex index) const
{
   return m_uvs[index];
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

BoundingBox InternalMesh::calculate_bouding_box() const
{
   BoundingBox result{
           { std::numeric_limits<float>::infinity(),  std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::infinity() },
           {-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(),
            -std::numeric_limits<float>::infinity()},
   };
   for (const auto vertex : m_mesh.vertices()) {
      const auto point = m_mesh.point(vertex);

      if (result.min.x > point.x()) {
         result.min.x = point.x();
      }
      if (result.min.y > point.y()) {
         result.min.y = point.y();
      }
      if (result.min.z > point.z()) {
         result.min.z = point.z();
      }

      if (result.max.x < point.x()) {
         result.max.x = point.x();
      }
      if (result.max.y < point.y()) {
         result.max.y = point.y();
      }
      if (result.max.z < point.z()) {
         result.max.z = point.z();
      }
   }

   return result;
}

InternalMesh InternalMesh::from_obj_file(io::IReader &stream)
{
   InternalMesh result;

   Parser parser(stream);
   parser.parse();

   std::vector<glm::vec3> normalPalette{};
   std::vector<glm::vec2> uvPalette{};

   Index lastGroupIndex = g_invalidIndex;

   for (const auto &[name, arguments] : parser.commands()) {
      if (name == "v") {
         assert(arguments.size() >= 3);
         result.add_vertex(
                 glm::vec3{std::stof(arguments[0]), -std::stof(arguments[1]), std::stof(arguments[2])});
      } else if (name == "vn") {
         assert(arguments.size() >= 3);
         normalPalette.emplace_back(std::stof(arguments[0]), -std::stof(arguments[1]),
                                    std::stof(arguments[2]));
      } else if (name == "vt") {
         assert(arguments.size() >= 2);
         uvPalette.emplace_back(std::stof(arguments[0]), 1 - std::stof(arguments[1]));
      } else if (name == "f") {
         std::vector<SurfaceMesh::vertex_index> vertexIds;
         std::vector<IndexedVertex> indices;

         for (const auto &attribute : arguments) {
            const auto index = parse_index(attribute);
            assert(index.uv <= uvPalette.size());
            assert(index.normal <= normalPalette.size());

            indices.push_back(index);
            SurfaceMesh::vertex_index vertexId{index.location - 1};
            vertexIds.push_back(vertexId);
         }

         auto faceIndex = result.add_face(vertexIds);
         if (faceIndex.id() == g_invalidIndex)
            continue;

         result.m_groupIds[faceIndex] = lastGroupIndex;

         int i = 0;
         for (const auto halfEdge : result.m_mesh.halfedges_around_face(result.m_mesh.halfedge(faceIndex))) {
            if (indices[i].normal != -1)
               result.m_normals[halfEdge] = normalPalette[indices[i].normal - 1];

            if (indices[i].uv != -1)
               result.m_uvs[halfEdge] = uvPalette[indices[i].uv - 1];

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

InternalMesh InternalMesh::from_obj_file(const io::Path& path)
{
   const auto file = io::open_file(path, io::FileOpenMode::Read);
   if (not file.has_value()) {
      throw std::runtime_error("failed to open object file");
   }
   return InternalMesh::from_obj_file(**file);
}

DeviceMesh InternalMesh::upload_to_device(graphics_api::Device &device)
{
   if (not this->is_triangulated())
      throw std::runtime_error("mesh must be triangulated before upload to GPU");
   assert(not m_mesh.faces().empty());

   std::unordered_map<Vertex, uint32_t> vertexMap{};
   std::vector<Vertex> outVertices{};

   std::vector<MaterialRange> materialRanges{};
   std::string currentMaterial;

   size_t lastOffset{};

   std::vector<uint32_t> outIndices{};
   for (const auto face_index : this->faces()) {
      const auto groupId = m_groupIds[face_index];
      if (groupId != g_invalidIndex) {
         const auto &group = m_groups[groupId];
         if (group.material != currentMaterial) {
            if (lastOffset != outIndices.size()) {
               materialRanges.push_back(
                       MaterialRange{lastOffset, outIndices.size() - lastOffset, currentMaterial});
            }
            currentMaterial = group.material;
            lastOffset      = outIndices.size();
         }
      }

      for (const auto halfedge_index : this->face_halfedges(face_index)) {
         const auto vertex_index = this->halfedge_target(halfedge_index);
         const auto normalVector = m_normals[halfedge_index].value_or(glm::vec3{0.0f, 1.0f, 0.0f});
         const auto tangent      = m_tangents[halfedge_index].value_or(Tangent{
                 glm::vec3{1.0f, 0.0f, 0.0f},
                 1.0f
         });

         Vertex vertex{
                 this->location(vertex_index),
                 m_uvs[halfedge_index].value_or(glm::vec2(0.0f, 0.0f)),
                 normalVector,
                 tangent.vector,
                 tangent.sign * glm::cross(normalVector, tangent.vector),
         };

         if (vertexMap.contains(vertex)) {
            outIndices.push_back(vertexMap[vertex]);
         } else {
            outVertices.push_back(vertex);
            vertexMap[vertex] = outVertices.size() - 1;
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

   return {
           {std::move(gpuVertices), std::move(gpuIndices)},
           std::move(materialRanges)
   };
}

void InternalMesh::reverse_orientation()
{
   CGAL::Polygon_mesh_processing::reverse_face_orientations(m_mesh);
   for (auto &normal : m_normals) {
      if (normal.has_value()) {
         normal = -*normal;
      }
   }
}

void InternalMesh::recalculate_tangents()
{
   SMikkTSpaceInterface interface{};
   interface.m_getNumFaces = [](const SMikkTSpaceContext *pContext) -> int {
      const auto *mesh = static_cast<InternalMesh *>(pContext->m_pUserData);
      return mesh->faces().size();
   };
   interface.m_getNumVerticesOfFace = [](const SMikkTSpaceContext *pContext, const int iFace) -> int {
      const auto *mesh = static_cast<InternalMesh *>(pContext->m_pUserData);
      const FaceIndex faceIndex{static_cast<FaceIndex::size_type>(iFace)};
      return mesh->m_mesh.vertices_around_face(mesh->m_mesh.halfedge(faceIndex)).size();
   };
   interface.m_getPosition = [](const SMikkTSpaceContext *pContext, float fvPosOut[], const int iFace,
                                const int iVert) {
      const auto *mesh = static_cast<InternalMesh *>(pContext->m_pUserData);
      const FaceIndex faceIndex{static_cast<FaceIndex::size_type>(iFace)};
      auto halfedge = mesh->m_mesh.halfedge(faceIndex);
      for (int i = 0; i < iVert; ++i) {
         halfedge = mesh->m_mesh.next(halfedge);
      }
      const auto vertexIndex = mesh->m_mesh.target(halfedge);
      const auto location    = mesh->location(vertexIndex);
      fvPosOut[0]            = location.x;
      fvPosOut[1]            = location.y;
      fvPosOut[2]            = location.z;
   };
   interface.m_getNormal = [](const SMikkTSpaceContext *pContext, float fvNormOut[], const int iFace,
                              const int iVert) {
      const auto *mesh = static_cast<InternalMesh *>(pContext->m_pUserData);
      const FaceIndex faceIndex{static_cast<FaceIndex::size_type>(iFace)};
      auto halfedge = mesh->m_mesh.halfedge(faceIndex);
      for (int i = 0; i < iVert; ++i) {
         halfedge = mesh->m_mesh.next(halfedge);
      }
      const auto normal = mesh->normal(halfedge);
      if (normal.has_value()) {
         fvNormOut[0] = normal->x;
         fvNormOut[1] = normal->y;
         fvNormOut[2] = normal->z;
      }
   };
   interface.m_getTexCoord = [](const SMikkTSpaceContext *pContext, float fvTexcOut[], const int iFace,
                                const int iVert) {
      const auto *mesh = static_cast<InternalMesh *>(pContext->m_pUserData);
      const FaceIndex faceIndex{static_cast<FaceIndex::size_type>(iFace)};
      auto halfedge = mesh->m_mesh.halfedge(faceIndex);
      for (int i = 0; i < iVert; ++i) {
         halfedge = mesh->m_mesh.next(halfedge);
      }
      const auto uv = mesh->uv(halfedge);
      if (uv.has_value()) {
         fvTexcOut[0] = uv->x;
         fvTexcOut[1] = uv->y;
      }
   };
   interface.m_setTSpaceBasic = [](const SMikkTSpaceContext *pContext, const float fvTangent[],
                                   const float fSign, const int iFace, const int iVert) {
      auto *mesh = static_cast<InternalMesh *>(pContext->m_pUserData);
      const FaceIndex faceIndex{static_cast<FaceIndex::size_type>(iFace)};
      auto halfedge = mesh->m_mesh.halfedge(faceIndex);
      for (int i = 0; i < iVert; ++i) {
         halfedge = mesh->m_mesh.next(halfedge);
      }
      mesh->m_tangents[halfedge] = Tangent{
              glm::vec3{fvTangent[0], fvTangent[1], fvTangent[2]},
              fSign
      };
   };

   SMikkTSpaceContext context{&interface, this};
   genTangSpaceDefault(&context);
}

}// namespace triglav::geometry
