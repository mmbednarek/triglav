#include "InternalMesh.hpp"

#include "Parser.hpp"

#include "triglav/io/File.hpp"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wdeprecated-copy-with-user-provided-copy"
#pragma clang diagnostic ignored "-Wdeprecated-literal-operator"
#elifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#pragma GCC diagnostic ignored "-Wdeprecated-literal-operator"
#elifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996 4456 4245 4267 4244 4305 4458 5054 4702 5030 5222)
#endif

#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>

#ifdef __clang__
#pragma clang diagnostic pop
#elifdef __GNUC__
#pragma GCC diagnostic pop
#elifdef _MSC_VER
#pragma warning(pop)
#endif

#include <glm/geometric.hpp>
#include <mikktspace/mikktspace.h>
#include <unordered_map>

namespace {

triglav::geometry::IndexedVertex parse_index(const std::string& index)
{
   using triglav::geometry::g_invalid_index;
   using triglav::geometry::Index;
   using triglav::geometry::IndexedVertex;

   const auto it1 = index.find('/');
   const auto vertex_id = static_cast<Index>(std::stoi(index.substr(0, it1)));
   if (it1 == std::string::npos) {
      return IndexedVertex{vertex_id, g_invalid_index, g_invalid_index};
   }

   const auto it2 = index.find('/', it1 + 1);
   if (it2 == std::string::npos) {
      const auto uv_id = static_cast<Index>(std::stoi(index.substr(it1 + 1)));
      return IndexedVertex{vertex_id, uv_id, g_invalid_index};
   }

   const auto uv_id = static_cast<Index>(std::stoi(index.substr(it1 + 1, it2 - it1 - 1)));
   const auto normal_id = static_cast<Index>(std::stoi(index.substr(it2 + 1)));

   return {vertex_id, uv_id, normal_id};
}

}// namespace

namespace triglav::geometry {

using namespace name_literals;

InternalMesh::InternalMesh() :
    m_normals(m_mesh.add_property_map<HalfedgeIndex, std::optional<glm::vec3>>("h:normals", std::nullopt).first),
    m_uvs(m_mesh.add_property_map<HalfedgeIndex, std::optional<glm::vec2>>("h:uvs", std::nullopt).first),
    m_group_ids(m_mesh.add_property_map<FaceIndex, Index>("f:groups", g_invalid_index).first),
    m_tangents(m_mesh.add_property_map<HalfedgeIndex, std::optional<glm::vec4>>("h:tangents", std::nullopt).first)
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
   return static_cast<Index>(m_groups.size() - 1);
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
   bool all_faces_fixed = false;
   int limit = 3;
   while (not all_faces_fixed) {
      if (limit == 0)
         break;
      --limit;

      all_faces_fixed = true;
      for (const auto face : m_mesh.faces()) {
         if (m_group_ids[face] != g_invalid_index)
            continue;

         auto orig_halfedge = m_mesh.halfedge(face);
         auto halfedge = m_mesh.prev_around_target(orig_halfedge);
         auto source_face = m_mesh.face(halfedge);
         while (halfedge != orig_halfedge && m_group_ids[source_face] == g_invalid_index) {
            halfedge = m_mesh.prev_around_target(halfedge);
            source_face = m_mesh.face(halfedge);
         }

         if (halfedge == orig_halfedge) {
            all_faces_fixed = false;
            continue;
         }

         m_group_ids[face] = m_group_ids[source_face];
      }
   }

   assert(all_faces_fixed);

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
      m_uvs[halfedge] = m_uvs[corresponding_halfedge];
   }

   m_is_triangulated = true;
}

void InternalMesh::recalculate_normals()
{
   const auto vertex_normals = m_mesh.add_property_map<VertexIndex, Vector3>("v:normals", Vector3(0, 0, 0)).first;
   CGAL::Polygon_mesh_processing::compute_vertex_normals(m_mesh, vertex_normals);

   for (const auto vertex : m_mesh.vertices()) {
      const auto normal = vertex_normals[vertex];
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

InternalMesh::SurfaceMesh::Halfedge_around_face_range InternalMesh::face_halfedges(const FaceIndex index) const
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

void InternalMesh::set_face_tangents(const Index face, const std::span<Vector4> tangents)
{
   auto it = tangents.begin();
   for (const auto halfedge : m_mesh.halfedges_around_face(m_mesh.halfedge(FaceIndex{face}))) {
      if (it == tangents.end())
         break;
      m_tangents[halfedge] = *it;
      ++it;
   }
}

void InternalMesh::set_face_group(const Index face, const Index group)
{
   m_group_ids[FaceIndex{face}] = group;
}

void InternalMesh::set_material(const Index mesh_group, const MaterialName material)
{
   m_groups[mesh_group].material = material;
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
   if (m_is_triangulated)
      return true;

   if (CGAL::is_triangle_mesh(m_mesh)) {
      m_is_triangulated = true;
      return true;
   }

   return false;
}

BoundingBox InternalMesh::calculate_bounding_box() const
{
   BoundingBox result{
      {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()},
      {-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()},
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

InternalMesh InternalMesh::from_obj_file(io::IReader& stream)
{
   InternalMesh result;

   Parser parser(stream);
   parser.parse();

   std::vector<glm::vec3> normal_palette{};
   std::vector<glm::vec2> uv_palette{};

   Index last_group_index = g_invalid_index;

   for (const auto& [name, arguments] : parser.commands()) {
      if (name == "v") {
         assert(arguments.size() >= 3);
         result.add_vertex(glm::vec3{std::stof(arguments[0]), -std::stof(arguments[1]), std::stof(arguments[2])});
      } else if (name == "vn") {
         assert(arguments.size() >= 3);
         normal_palette.emplace_back(std::stof(arguments[0]), -std::stof(arguments[1]), std::stof(arguments[2]));
      } else if (name == "vt") {
         assert(arguments.size() >= 2);
         uv_palette.emplace_back(std::stof(arguments[0]), 1 - std::stof(arguments[1]));
      } else if (name == "f") {
         std::vector<SurfaceMesh::vertex_index> vertex_ids;
         std::vector<IndexedVertex> indices;

         for (const auto& attribute : arguments) {
            const auto index = parse_index(attribute);
            assert(index.uv <= uv_palette.size());
            assert(index.normal <= normal_palette.size());

            indices.push_back(index);
            SurfaceMesh::vertex_index vertex_id{index.location - 1};
            vertex_ids.push_back(vertex_id);
         }

         auto face_index = result.add_face(vertex_ids);
         if (face_index.id() == g_invalid_index)
            continue;

         result.m_group_ids[face_index] = last_group_index;

         int i = 0;
         for (const auto half_edge : result.m_mesh.halfedges_around_face(result.m_mesh.halfedge(face_index))) {
            if (indices[i].normal != ~0u)
               result.m_normals[half_edge] = normal_palette[indices[i].normal - 1];

            if (indices[i].uv != ~0u)
               result.m_uvs[half_edge] = uv_palette[indices[i].uv - 1];

            ++i;
         }
      } else if (name == "o") {
         assert(arguments.size() == 1);
         last_group_index = result.add_group({arguments[0], "material/stone.mat"_rc});
      } else if (name == "usemtl") {
         assert(arguments.size() == 1);
         if (not result.m_groups.empty()) {
            result.m_groups[last_group_index].material = make_rc_name(arguments[0]);
         }
      }
   }

   return result;
}

InternalMesh InternalMesh::from_obj_file(const io::Path& path)
{
   const auto file = io::open_file(path, io::FileMode::Read);
   if (not file.has_value()) {
      throw std::runtime_error("failed to open object file");
   }
   return InternalMesh::from_obj_file(**file);
}

DeviceMesh InternalMesh::upload_to_device(graphics_api::Device& device, const graphics_api::BufferUsageFlags usage_flags)
{
   auto vertex_data = this->to_vertex_data();

   graphics_api::VertexArray<Vertex> gpu_vertices{device, vertex_data.vertices.size(), usage_flags};
   GAPI_CHECK_STATUS(gpu_vertices.write(vertex_data.vertices.data(), vertex_data.vertices.size()));

   graphics_api::IndexArray gpu_indices{device, vertex_data.indices.size(), usage_flags};
   GAPI_CHECK_STATUS(gpu_indices.write(vertex_data.indices.data(), vertex_data.indices.size()));

   return {{std::move(gpu_vertices), std::move(gpu_indices)}, std::move(vertex_data.ranges)};
}

VertexData InternalMesh::to_vertex_data()
{
   if (not this->is_triangulated())
      throw std::runtime_error("mesh must be triangulated before calculating vertex data");
   assert(not m_mesh.faces().empty());

   std::unordered_map<Vertex, u32> vertex_map{};
   std::vector<Vertex> out_vertices{};

   std::vector<MaterialRange> material_ranges{};
   MaterialName current_material;

   size_t last_offset{};

   std::vector<uint32_t> out_indices{};
   for (const auto face_index : this->faces()) {
      const auto group_id = m_group_ids[face_index];
      if (group_id != g_invalid_index) {
         const auto& group = m_groups[group_id];
         if (group.material != current_material) {
            if (last_offset != out_indices.size()) {
               material_ranges.push_back(MaterialRange{last_offset, out_indices.size() - last_offset, current_material});
            }
            current_material = group.material;
            last_offset = out_indices.size();
         }
      }

      for (const auto halfedge_index : this->face_halfedges(face_index)) {
         const auto vertex_index = this->halfedge_target(halfedge_index);
         const auto normal_vector = m_normals[halfedge_index].value_or(glm::vec3{0.0f, 1.0f, 0.0f});
         const auto tangent = m_tangents[halfedge_index].value_or(glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});

         Vertex vertex{
            this->location(vertex_index),
            m_uvs[halfedge_index].value_or(glm::vec2(0.0f, 0.0f)),
            normal_vector,
            tangent,
         };

         if (vertex_map.contains(vertex)) {
            out_indices.push_back(vertex_map[vertex]);
         } else {
            out_vertices.push_back(vertex);
            vertex_map[vertex] = static_cast<u32>(out_vertices.size() - 1);
            out_indices.push_back(static_cast<u32>(out_vertices.size() - 1));
         }
      }
   }

   if (last_offset != out_indices.size()) {
      material_ranges.push_back(MaterialRange{last_offset, out_indices.size() - last_offset, current_material});
   }

   return {.vertices{std::move(out_vertices)}, .indices{std::move(out_indices)}, .ranges{std::move(material_ranges)}};
}

void InternalMesh::reverse_orientation()
{
   CGAL::Polygon_mesh_processing::reverse_face_orientations(m_mesh);
   for (auto& normal : m_normals) {
      if (normal.has_value()) {
         normal = -*normal;
      }
   }
}

void InternalMesh::recalculate_tangents()
{
   SMikkTSpaceInterface interface{};
   interface.m_getNumFaces = [](const SMikkTSpaceContext* pContext) -> int {
      const auto* mesh = static_cast<InternalMesh*>(pContext->m_pUserData);
      return static_cast<int>(mesh->faces().size());
   };
   interface.m_getNumVerticesOfFace = [](const SMikkTSpaceContext* pContext, const int iFace) -> int {
      const auto* mesh = static_cast<InternalMesh*>(pContext->m_pUserData);
      const FaceIndex faceIndex{static_cast<FaceIndex::size_type>(iFace)};
      return static_cast<int>(mesh->m_mesh.vertices_around_face(mesh->m_mesh.halfedge(faceIndex)).size());
   };
   interface.m_getPosition = [](const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert) {
      const auto* mesh = static_cast<InternalMesh*>(pContext->m_pUserData);
      const FaceIndex faceIndex{static_cast<FaceIndex::size_type>(iFace)};
      auto halfedge = mesh->m_mesh.halfedge(faceIndex);
      for (int i = 0; i < iVert; ++i) {
         halfedge = mesh->m_mesh.next(halfedge);
      }
      const auto vertexIndex = mesh->m_mesh.target(halfedge);
      const auto location = mesh->location(vertexIndex);
      fvPosOut[0] = location.x;
      fvPosOut[1] = location.y;
      fvPosOut[2] = location.z;
   };
   interface.m_getNormal = [](const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert) {
      const auto* mesh = static_cast<InternalMesh*>(pContext->m_pUserData);
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
   interface.m_getTexCoord = [](const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert) {
      const auto* mesh = static_cast<InternalMesh*>(pContext->m_pUserData);
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
   interface.m_setTSpaceBasic = [](const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace,
                                   const int iVert) {
      auto* mesh = static_cast<InternalMesh*>(pContext->m_pUserData);
      const FaceIndex faceIndex{static_cast<FaceIndex::size_type>(iFace)};
      auto halfedge = mesh->m_mesh.halfedge(faceIndex);
      for (int i = 0; i < iVert; ++i) {
         halfedge = mesh->m_mesh.next(halfedge);
      }
      mesh->m_tangents[halfedge] = glm::vec4{fvTangent[0], fvTangent[1], fvTangent[2], fSign};
   };

   SMikkTSpaceContext context{&interface, this};
   genTangSpaceDefault(&context);
}

}// namespace triglav::geometry
