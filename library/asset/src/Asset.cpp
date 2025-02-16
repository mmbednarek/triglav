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
};

struct MeshGroupInfo
{
   u32 indexOffset;
   u32 indexCount;
   MaterialName materialName;
};

bool encode_mesh(const geometry::Mesh& mesh, io::IWriter& writer)
{
   AssetHeader header{};
   header.magic = g_magicNumber;
   header.version = g_lastVersion;
   header.type = ResourceType::Model;
   if (!writer.write({reinterpret_cast<const u8*>(&header), sizeof(AssetHeader)}).has_value()) {
      return false;
   }

   auto vertexData = mesh.to_vertex_data();

   MeshHeader meshHeader{};
   meshHeader.vertexLayout = MeshVertexLayout::NormalMapped;
   meshHeader.vertexCount = vertexData.vertices.size();
   meshHeader.indexCount = vertexData.indices.size();
   meshHeader.groupCount = vertexData.ranges.size();
   if (!writer.write({reinterpret_cast<const u8*>(&meshHeader), sizeof(MeshHeader)}).has_value()) {
      return false;
   }

   for (const auto& range : vertexData.ranges) {
      MeshGroupInfo groupInfo{
         .indexOffset = static_cast<u32>(range.offset),
         .indexCount = static_cast<u32>(range.size),
         .materialName = make_rc_name(fmt::format("{}.mat", range.materialName)),
      };
      if (!writer.write({reinterpret_cast<const u8*>(&groupInfo), sizeof(MeshGroupInfo)}).has_value()) {
         return false;
      }
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

}// namespace triglav::asset
