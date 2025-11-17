#include "Asset.hpp"

#include "triglav/Int.hpp"
#include "triglav/Math.hpp"
#include "triglav/ResourceType.hpp"
#include "triglav/io/DisplacedStream.hpp"
#include "triglav/ktx/Texture.hpp"

namespace triglav::asset {

constexpr u32 g_magic_number = 0x53414754;
constexpr u32 g_last_version = 0x2500;

enum class MeshVertexLayout : u32
{
   Standard,    // Position: float3, Normal: float3, TexCoord: float2
   NormalMapped,// Position: float3, Normal: float3, TexCoord: float2, Tangent: float4
   SkeletalMesh,
};

struct MeshHeader
{
   MeshVertexLayout vertex_layout;
   u32 vertex_count;
   u32 index_count;
   u32 group_count;
   Vector3 bounding_box_min;
   Vector3 bounding_box_max;
};

namespace {

bool write_header(io::IWriter& writer, const ResourceType resource_type)
{
   AssetHeader header{};
   header.magic = g_magic_number;
   header.version = g_last_version;
   header.type = resource_type;
   return writer.write({reinterpret_cast<const u8*>(&header), sizeof(AssetHeader)}).has_value();
}


}// namespace

EncodedSamplerProperties encode_sampler_properties(const SamplerProperties& properties)
{
   return static_cast<u32>(properties.min_filter) | (static_cast<u32>(properties.mag_filter) << 1) |
          (static_cast<u32>(properties.address_mode_u) << 2) | (static_cast<u32>(properties.address_mode_v) << 5) |
          (static_cast<u32>(properties.address_mode_w) << 8) | (static_cast<u32>(properties.enable_anisotropy) << 11);
}

SamplerProperties decode_sampler_properties(const EncodedSamplerProperties encoded_properties)
{
   SamplerProperties result{};
   result.min_filter = static_cast<FilterType>(encoded_properties & 0b1);
   result.mag_filter = static_cast<FilterType>((encoded_properties >> 1) & 0b1);
   result.address_mode_u = static_cast<TextureAddressMode>((encoded_properties >> 2) & 0b111);
   result.address_mode_v = static_cast<TextureAddressMode>((encoded_properties >> 5) & 0b111);
   result.address_mode_w = static_cast<TextureAddressMode>((encoded_properties >> 8) & 0b111);
   result.enable_anisotropy = static_cast<bool>((encoded_properties >> 11) & 0b1);
   return result;
}

std::optional<AssetHeader> decode_header(io::IReader& reader)
{
   AssetHeader header{};
   if (!reader.read({reinterpret_cast<u8*>(&header), sizeof(AssetHeader)}).has_value()) {
      return std::nullopt;
   }

   if (header.magic != g_magic_number) {
      return std::nullopt;
   }
   if (header.version > g_last_version) {
      return std::nullopt;
   }

   return header;
}

bool encode_mesh(io::IWriter& writer, const geometry::Mesh& mesh)
{
   write_header(writer, ResourceType::Mesh);

   const auto bb = mesh.calculate_bounding_box();
   const auto vertex_data = mesh.to_vertex_data();

   MeshHeader mesh_header{};
   mesh_header.vertex_layout = MeshVertexLayout::NormalMapped;
   mesh_header.vertex_count = static_cast<u32>(vertex_data.vertices.size());
   mesh_header.index_count = static_cast<u32>(vertex_data.indices.size());
   mesh_header.group_count = static_cast<u32>(vertex_data.ranges.size());
   mesh_header.bounding_box_min = bb.min;
   mesh_header.bounding_box_max = bb.max;
   if (!writer.write({reinterpret_cast<const u8*>(&mesh_header), sizeof(MeshHeader)}).has_value()) {
      return false;
   }

   if (!writer.write({reinterpret_cast<const u8*>(vertex_data.ranges.data()), sizeof(geometry::MaterialRange) * vertex_data.ranges.size()})
           .has_value()) {
      return false;
   }

   if (!writer.write({reinterpret_cast<const u8*>(vertex_data.vertices.data()), sizeof(geometry::Vertex) * vertex_data.vertices.size()})
           .has_value()) {
      return false;
   }

   if (!writer.write({reinterpret_cast<const u8*>(vertex_data.indices.data()), sizeof(u32) * vertex_data.indices.size()}).has_value()) {
      return false;
   }

   return true;
}

std::optional<geometry::MeshData> decode_mesh(io::IReader& reader)
{
   MeshHeader mesh_header{};
   if (!reader.read({reinterpret_cast<u8*>(&mesh_header), sizeof(MeshHeader)}).has_value()) {
      return std::nullopt;
   }

   if (mesh_header.vertex_layout != MeshVertexLayout::NormalMapped) {
      return std::nullopt;
   }

   geometry::MeshData mesh_data{};
   mesh_data.bounding_box.min = mesh_header.bounding_box_min;
   mesh_data.bounding_box.max = mesh_header.bounding_box_max;

   mesh_data.vertex_data.ranges.resize(mesh_header.group_count);
   if (!reader.read({reinterpret_cast<u8*>(mesh_data.vertex_data.ranges.data()), sizeof(geometry::MaterialRange) * mesh_header.group_count})
           .has_value()) {
      return std::nullopt;
   }

   mesh_data.vertex_data.vertices.resize(mesh_header.vertex_count);
   if (!reader.read({reinterpret_cast<u8*>(mesh_data.vertex_data.vertices.data()), sizeof(geometry::Vertex) * mesh_header.vertex_count})
           .has_value()) {
      return std::nullopt;
   }

   mesh_data.vertex_data.indices.resize(mesh_header.index_count);
   if (!reader.read({reinterpret_cast<u8*>(mesh_data.vertex_data.indices.data()), sizeof(u32) * mesh_header.index_count}).has_value()) {
      return std::nullopt;
   }

   return mesh_data;
}

bool encode_texture(io::IWriter& writer, const TexturePurpose purpose, const ktx::Texture& tex, const SamplerProperties& sampler)
{
   write_header(writer, ResourceType::Texture);

   TextureHeader tex_header{};
   tex_header.format = tex.is_compressed() ? TextureFormat::KTX_CompressedBC3 : TextureFormat::KTX_Uncompressed;
   tex_header.purpose = purpose;
   tex_header.payload_size = 0;
   tex_header.sampler_properties = encode_sampler_properties(sampler);

   if (!writer.write({reinterpret_cast<const u8*>(&tex_header), sizeof(TextureHeader)}).has_value()) {
      return false;
   }

   return tex.write_to_stream(writer);
}

std::optional<DecodedTexture> decode_texture(io::IFile& stream)
{
   TextureHeader tex_header{};
   if (!stream.read({reinterpret_cast<u8*>(&tex_header), sizeof(TextureHeader)}).has_value()) {
      return std::nullopt;
   }

   static constexpr auto offset = sizeof(AssetHeader) + sizeof(TextureHeader);
   io::DisplacedStream displaced_stream{stream, offset, *stream.file_size() - offset};

   auto tex_result = ktx::Texture::from_stream(displaced_stream);
   if (!tex_result.has_value()) {
      return std::nullopt;
   }

   return DecodedTexture{std::move(*tex_result), tex_header.format, tex_header.purpose,
                         decode_sampler_properties(tex_header.sampler_properties)};
}

}// namespace triglav::asset
