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
std::vector<TAccessorType> accessor_to_array(const u32 accessorId, const Document& doc, const BufferManager& bufferManager)
{
   auto& accessor = doc.accessors.at(accessorId);
   if (std::is_same_v<TAccessorType, u16>) {
      assert(accessor.componentType == ComponentType::UnsignedShort);
   } else {
      assert(accessor.componentType == ComponentType::Float);
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

   auto bufferView = bufferManager.read_buffer_view(accessor.bufferView, accessor.byteOffset);
   for (const u32 i : Range(0u, accessor.count)) {
      result[i] = bufferView.read_value<TAccessorType>();
   }

   return result;
}

template<typename TVector>
std::vector<TVector> extract_vector(const PrimitiveAttributeType type, const Document& doc, const Primitive& prim,
                                    const BufferManager& bufferManager)
{
   const auto itPos = std::ranges::find_if(prim.attributes, [type](const PrimitiveAttribute& attrib) { return attrib.type == type; });
   if (itPos == prim.attributes.end()) {
      return {};
   }
   return accessor_to_array<TVector>(itPos->accessorId, doc, bufferManager);
}

}// namespace

geometry::Mesh mesh_from_document(const Document& doc, const u32 meshIndex, const BufferManager& bufferManager,
                                  const std::map<u32, MaterialName>& materials)
{
   const Mesh& srcMesh = doc.meshes[meshIndex];

   geometry::Mesh dstMesh;

   const auto invalidGroupID = dstMesh.add_group(geometry::MeshGroup{.name = "invalid", .material = "stone.mat"_rc});

   std::map<u32, geometry::Index> indexToName;
   for (const auto& [index, name] : materials) {
      indexToName[index] = dstMesh.add_group(geometry::MeshGroup{.name = std::format("group{}", index), .material = name});
   }

   std::unordered_map<Vector3, geometry::Index> uniquePositionMap;

   for (const auto& prim : srcMesh.primitives) {
      if (!prim.indices.has_value())
         continue;

      auto groupID = invalidGroupID;
      if (prim.material.has_value() && indexToName.contains(prim.material.value())) {
         groupID = indexToName.at(prim.material.value());
      }

      auto positions = extract_vector<Vector3>(PrimitiveAttributeType::Position, doc, prim, bufferManager);
      for (const auto& position : positions) {
         if (uniquePositionMap.contains(position))
            continue;
         auto index = dstMesh.add_vertex(position);
         uniquePositionMap.emplace(position, index);
      }

      auto normals = extract_vector<Vector3>(PrimitiveAttributeType::Normal, doc, prim, bufferManager);
      auto uvs = extract_vector<Vector2>(PrimitiveAttributeType::TexCoord, doc, prim, bufferManager);
      auto indices = accessor_to_array<u16>(*prim.indices, doc, bufferManager);
      auto indicesIt = indices.begin();
      while (indicesIt != indices.end()) {
         const auto a = *indicesIt++;
         const auto b = *indicesIt++;
         const auto c = *indicesIt++;
         const auto faceId =
            dstMesh.add_face(uniquePositionMap[positions[c]], uniquePositionMap[positions[b]], uniquePositionMap[positions[a]]);
         dstMesh.set_face_normals(faceId, normals[c], normals[b], normals[a]);
         dstMesh.set_face_uvs(faceId, uvs[c], uvs[b], uvs[a]);
         dstMesh.set_face_group(faceId, groupID);
      }
   }

   // dstMesh.reverse_orientation();

   return dstMesh;
}

std::optional<geometry::Mesh> load_glb_mesh(const io::Path& path)
{
   const auto glbRes = open_glb_file(path);
   if (!glbRes.has_value()) {
      return std::nullopt;
   }

   return mesh_from_document(*glbRes->document, 0, glbRes->bufferManager, {});
}

}// namespace triglav::gltf