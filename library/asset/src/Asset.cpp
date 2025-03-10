#include "Asset.hpp"

#include "triglav/Int.hpp"
#include "triglav/Math.hpp"
#include "triglav/ResourceType.hpp"
#include "triglav/io/DisplacedStream.hpp"
#include "triglav/ktx/Texture.hpp"

#include <fmt/core.h>

namespace triglav::asset {

constexpr u32 g_magicNumber = 0x53414754;
constexpr u32 g_lastVersion = 0x2500;

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

namespace {

bool write_header(io::IWriter& writer, const ResourceType resourceType)
{
   AssetHeader header{};
   header.magic = g_magicNumber;
   header.version = g_lastVersion;
   header.type = resourceType;
   return writer.write({reinterpret_cast<const u8*>(&header), sizeof(AssetHeader)}).has_value();
}


}// namespace

EncodedSamplerProperties encode_sampler_properties(const SamplerProperties& properties)
{
   return static_cast<u32>(properties.minFilter) | (static_cast<u32>(properties.magFilter) << 1) |
          (static_cast<u32>(properties.addressModeU) << 2) | (static_cast<u32>(properties.addressModeV) << 5) |
          (static_cast<u32>(properties.addressModeW) << 8) | (static_cast<u32>(properties.enableAnisotropy) << 11);
}

SamplerProperties decode_sampler_properties(const EncodedSamplerProperties encodedProperties)
{
   SamplerProperties result{};
   result.minFilter = static_cast<FilterType>(encodedProperties & 0b1);
   result.magFilter = static_cast<FilterType>((encodedProperties >> 1) & 0b1);
   result.addressModeU = static_cast<TextureAddressMode>((encodedProperties >> 2) & 0b111);
   result.addressModeV = static_cast<TextureAddressMode>((encodedProperties >> 5) & 0b111);
   result.addressModeW = static_cast<TextureAddressMode>((encodedProperties >> 8) & 0b111);
   result.enableAnisotropy = static_cast<bool>((encodedProperties >> 11) & 0b1);
   return result;
}

std::optional<AssetHeader> decode_header(io::IReader& reader)
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

   return header;
}

bool encode_mesh(io::IWriter& writer, const geometry::Mesh& mesh)
{
   write_header(writer, ResourceType::Mesh);

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

bool encode_texture(io::IWriter& writer, const TexturePurpose purpose, const ktx::Texture& tex, const SamplerProperties& sampler)
{
   write_header(writer, ResourceType::Texture);

   TextureHeader texHeader{};
   texHeader.format = tex.is_compressed() ? TextureFormat::KTX_CompressedBC3 : TextureFormat::KTX_Uncompressed;
   texHeader.purpose = purpose;
   texHeader.payloadSize = 0;
   texHeader.samplerProperties = encode_sampler_properties(sampler);

   if (!writer.write({reinterpret_cast<const u8*>(&texHeader), sizeof(TextureHeader)}).has_value()) {
      return false;
   }

   return tex.write_to_stream(writer);
}

std::optional<DecodedTexture> decode_texture(io::IFile& stream)
{
   TextureHeader texHeader{};
   if (!stream.read({reinterpret_cast<u8*>(&texHeader), sizeof(TextureHeader)}).has_value()) {
      return std::nullopt;
   }

   static constexpr auto offset = sizeof(AssetHeader) + sizeof(TextureHeader);
   io::DisplacedStream displacedStream{stream, offset, *stream.file_size() - offset};

   auto texResult = ktx::Texture::from_stream(displacedStream);
   if (!texResult.has_value()) {
      return std::nullopt;
   }

   return DecodedTexture{std::move(*texResult), texHeader.format, texHeader.purpose,
                         decode_sampler_properties(texHeader.samplerProperties)};
}

}// namespace triglav::asset
