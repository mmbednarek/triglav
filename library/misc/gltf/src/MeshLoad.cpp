#include "MeshLoad.hpp"

#include "BufferManager.hpp"
#include "Glb.hpp"
#include "Gltf.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/io/LimitedReader.hpp"

#include <format>
#include <ranges>
#include <unordered_map>

namespace triglav::gltf {

using namespace name_literals;

namespace {

template<typename TAccessorType>
std::vector<TAccessorType> accessor_to_array(const u32 accessor_id, const Document& doc, const BufferManager& buffer_manager)
{
   auto& accessor = doc.accessors.at(accessor_id);
   if (std::is_same_v<TAccessorType, u16>) {
      assert(accessor.component_type == ComponentType::UnsignedShort);
   } else {
      assert(accessor.component_type == ComponentType::Float);
   }
   if constexpr (std::is_same_v<TAccessorType, Vector2>) {
      assert(accessor.type == AccessorType::Vector2);
   } else if constexpr (std::is_same_v<TAccessorType, Vector3>) {
      assert(accessor.type == AccessorType::Vector3);
   } else if constexpr (std::is_same_v<TAccessorType, Vector4>) {
      assert(accessor.type == AccessorType::Vector4);
   } else if constexpr (std::is_same_v<TAccessorType, u16>) {
      assert(accessor.type == AccessorType::Scalar);
   }

   std::vector<TAccessorType> result;
   result.resize(accessor.count);

   auto buffer_view = buffer_manager.read_buffer_view(accessor.buffer_view, accessor.byte_offset);
   for (const u32 i : Range(0u, accessor.count)) {
      result[i] = buffer_view.read_value<TAccessorType>();
   }

   return result;
}

template<typename TVector>
std::vector<TVector> extract_vector(const PrimitiveAttributeType type, const Document& doc, const Primitive& prim,
                                    const BufferManager& buffer_manager)
{
   const auto it_pos = std::ranges::find_if(prim.attributes, [type](const PrimitiveAttribute& attrib) { return attrib.type == type; });
   if (it_pos == prim.attributes.end()) {
      return {};
   }
   return accessor_to_array<TVector>(it_pos->accessor_id, doc, buffer_manager);
}

}// namespace

geometry::Mesh mesh_from_document(const Document& doc, const u32 mesh_index, const BufferManager& buffer_manager,
                                  const std::map<u32, MaterialName>& materials)
{
   const Mesh& src_mesh = doc.meshes[mesh_index];

   geometry::Mesh dst_mesh;

   const auto invalid_group_id = dst_mesh.add_group(geometry::MeshGroup{.name = "invalid", .material = "material/stone.mat"_rc});

   std::map<u32, geometry::Index> index_to_name;
   for (const auto& [index, name] : materials) {
      index_to_name[index] = dst_mesh.add_group(geometry::MeshGroup{.name = std::format("group{}", index), .material = name});
   }

   std::unordered_map<Vector3, geometry::Index> unique_position_map;

   [[maybe_unused]] u32 prim_id = 0;
   for (const auto& prim : src_mesh.primitives) {
      if (!prim.indices.has_value())
         continue;

      auto group_id = invalid_group_id;
      if (prim.material.has_value() && index_to_name.contains(prim.material.value())) {
         group_id = index_to_name.at(prim.material.value());
      }

      auto positions = extract_vector<Vector3>(PrimitiveAttributeType::Position, doc, prim, buffer_manager);
      for (const auto& position : positions) {
         if (unique_position_map.contains(position))
            continue;
         auto index = dst_mesh.add_vertex(position);
         unique_position_map.insert(std::make_pair(position, index));
         assert(!unique_position_map.empty());
      }

      auto normals = extract_vector<Vector3>(PrimitiveAttributeType::Normal, doc, prim, buffer_manager);
      auto uvs = extract_vector<Vector2>(PrimitiveAttributeType::TexCoord, doc, prim, buffer_manager);
      auto indices = accessor_to_array<u16>(*prim.indices, doc, buffer_manager);

      std::vector<Vector4> tangents{};
      tangents = extract_vector<Vector4>(PrimitiveAttributeType::Tangent, doc, prim, buffer_manager);

      auto indices_it = indices.begin();
      while (indices_it != indices.end()) {
         const auto a = *indices_it++;
         const auto b = *indices_it++;
         const auto c = *indices_it++;
         const auto face_id =
            dst_mesh.add_face(unique_position_map[positions[c]], unique_position_map[positions[b]], unique_position_map[positions[a]]);
         if (face_id == ~0u)
            continue;
         dst_mesh.set_face_normals(face_id, normals[c], normals[b], normals[a]);
         dst_mesh.set_face_uvs(face_id, uvs[c], uvs[b], uvs[a]);
         dst_mesh.set_face_group(face_id, group_id);
         if (!tangents.empty()) {
            dst_mesh.set_face_tangents(face_id, tangents[c], tangents[b], tangents[a]);
         }
      }
      ++prim_id;
   }

   return dst_mesh;
}

std::optional<geometry::Mesh> load_glb_mesh(const io::Path& path)
{
   const auto glb_res = open_glb_file(path);
   if (!glb_res.has_value()) {
      return std::nullopt;
   }

   return mesh_from_document(*glb_res->document, 0, glb_res->buffer_manager, {});
}

}// namespace triglav::gltf