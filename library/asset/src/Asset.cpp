#include "Asset.hpp"

#include "triglav/Int.hpp"
#include "triglav/Math.hpp"
#include "triglav/Name.hpp"
#include "triglav/ResourceType.hpp"

#include <fmt/core.h>

namespace triglav::asset {

constexpr u32 g_magicNumber = 0x53414754;
constexpr u32 g_lastVersion = 0x2500;

struct AssetHeader
{
   u32 magic;// 0x53414754
   u32 version;
   ResourceType type;
};

enum class MeshVertexLayout : u32
{
   Standard,    // Position: float3, Normal: float3, TexCoord: float2
   NormalMapped,// Position: float3, Normal: float3, TexCoord: float2, Tangent: float4
   SkeletalMesh,
};

struct MeshHeader
{
   MeshVertexLayout vertexLayout;
   u32 vertexCount;
   u32 indexCount;
   u32 groupCount;
   Vector3 boundingBoxMin;
   Vector3 boundingBoxMax;
};

bool encode_mesh(const geometry::Mesh& mesh, io::IWriter& writer)
{
   AssetHeader header{};
   header.magic = g_magicNumber;
   header.version = g_lastVersion;
   header.type = ResourceType::Mesh;
   if (!writer.write({reinterpret_cast<const u8*>(&header), sizeof(AssetHeader)}).has_value()) {
      return false;
   }

   const auto bb = mesh.calculate_bounding_box();
   const auto vertexData = mesh.to_vertex_data();

   MeshHeader meshHeader{};
   meshHeader.vertexLayout = MeshVertexLayout::NormalMapped;
   meshHeader.vertexCount = vertexData.vertices.size();
   meshHeader.indexCount = vertexData.indices.size();
   meshHeader.groupCount = vertexData.ranges.size();
   meshHeader.boundingBoxMin = bb.min;
   meshHeader.boundingBoxMax = bb.max;
   if (!writer.write({reinterpret_cast<const u8*>(&meshHeader), sizeof(MeshHeader)}).has_value()) {
      return false;
   }

   if (!writer.write({reinterpret_cast<const u8*>(vertexData.ranges.data()), sizeof(geometry::MaterialRange) * vertexData.ranges.size()})
           .has_value()) {
      return false;
   }

   if (!writer.write({reinterpret_cast<const u8*>(vertexData.vertices.data()), sizeof(geometry::Vertex) * vertexData.vertices.size()})
           .has_value()) {
      return false;
   }

   if (!writer.write({reinterpret_cast<const u8*>(vertexData.indices.data()), sizeof(u32) * vertexData.indices.size()}).has_value()) {
      return false;
   }

   return true;
}

std::optional<geometry::MeshData> decode_mesh(io::IReader& reader)
{
   AssetHeader header{};
   if (!reader.read({reinterpret_cast<u8*>(&header), sizeof(AssetHeader)}).has_value()) {
      return std::nullopt;
   }

   if (header.magic != g_magicNumber) {
      return std::nullopt;
   }
   if (header.version > g_lastVersion) {
      return std::nullopt;
   }
   if (header.type != ResourceType::Mesh) {
      return std::nullopt;
   }

   MeshHeader meshHeader{};
   if (!reader.read({reinterpret_cast<u8*>(&meshHeader), sizeof(MeshHeader)}).has_value()) {
      return std::nullopt;
   }

   if (meshHeader.vertexLayout != MeshVertexLayout::NormalMapped) {
      return std::nullopt;
   }

   geometry::MeshData meshData{};
   meshData.boundingBox.min = meshHeader.boundingBoxMin;
   meshData.boundingBox.max = meshHeader.boundingBoxMax;

   meshData.vertexData.ranges.resize(meshHeader.groupCount);
   if (!reader.read({reinterpret_cast<u8*>(meshData.vertexData.ranges.data()), sizeof(geometry::MaterialRange) * meshHeader.groupCount})
           .has_value()) {
      return std::nullopt;
   }

   meshData.vertexData.vertices.resize(meshHeader.vertexCount);
   if (!reader.read({reinterpret_cast<u8*>(meshData.vertexData.vertices.data()), sizeof(geometry::Vertex) * meshHeader.vertexCount})
           .has_value()) {
      return std::nullopt;
   }

   meshData.vertexData.indices.resize(meshHeader.indexCount);
   if (!reader.read({reinterpret_cast<u8*>(meshData.vertexData.indices.data()), sizeof(u32) * meshHeader.indexCount}).has_value()) {
      return std::nullopt;
   }

   return meshData;
}

}// namespace triglav::asset
