#include "MeshLoad.hpp"

#include "BufferManager.hpp"
#include "Glb.hpp"
#include "Gltf.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/io/LimitedReader.hpp"

#include <fmt/core.h>
#include <ranges>
#include <unordered_map>

namespace triglav::gltf {

namespace {

constexpr Vector3 g_flipY = Vector3{-1, -1, -1};

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

geometry::Mesh mesh_from_document(const Document& doc, const u32 meshIndex, const BufferManager& bufferManager)
{
   const Mesh& srcMesh = doc.meshes[meshIndex];

   geometry::Mesh dstMesh;

   const auto groupId = dstMesh.add_group(geometry::MeshGroup{.name = "main", .material = "stone"});

   std::unordered_map<Vector3, geometry::Index> uniquePositionMap;

   for (const auto& prim : srcMesh.primitives) {
      if (!prim.indices.has_value())
         continue;

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
            dstMesh.add_face(uniquePositionMap[positions[a]], uniquePositionMap[positions[b]], uniquePositionMap[positions[c]]);
         dstMesh.set_face_normals(faceId, normals[a] * g_flipY, normals[b] * g_flipY, normals[c] * g_flipY);
         dstMesh.set_face_uvs(faceId, uvs[a], uvs[b], uvs[c]);
         dstMesh.set_face_group(faceId, groupId);
      }
   }

   dstMesh.reverse_orientation();

   return dstMesh;
}

std::optional<geometry::Mesh> load_glb_mesh(const io::Path& path)
{
   auto fileHandleRes = io::open_file(path, io::FileOpenMode::Read);
   if (!fileHandleRes.has_value()) {
      return std::nullopt;
   }
   auto& fileHandle = **fileHandleRes;

   auto glbInfo = read_glb_info(fileHandle);
   if (!glbInfo.has_value()) {
      return std::nullopt;
   }

   fileHandle.seek(io::SeekPosition::Begin, static_cast<MemoryOffset>(glbInfo->jsonOffset));

   io::LimitedReader jsonReader(fileHandle, glbInfo->jsonSize);

   Document doc;
   doc.deserialize(jsonReader);

   BufferManager manager(doc, &fileHandle, static_cast<MemoryOffset>(glbInfo->binaryOffset));

   return mesh_from_document(doc, 0, manager);
}

}// namespace triglav::gltf