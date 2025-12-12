#include "Asset.hpp"

#include "triglav/Int.hpp"
#include "triglav/Math.hpp"
#include "triglav/ResourcePathMap.hpp"
#include "triglav/ResourceType.hpp"
#include "triglav/String.hpp"
#include "triglav/io/Deserializer.hpp"
#include "triglav/io/DisplacedStream.hpp"
#include "triglav/ktx/Texture.hpp"

#include <triglav/io/Serializer.hpp>

namespace triglav::asset {

using namespace string_literals;

constexpr u32 g_magic_number = 0x53414754;
constexpr u32 g_last_version = 0x2501;

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
   const auto bb = mesh.calculate_bounding_box();
   const auto vertex_data = mesh.to_vertex_data();
   encode_mesh_data(writer, geometry::MeshData{
                               .vertex_data = vertex_data,
                               .bounding_box = bb,
                            });
   return true;
}

bool encode_mesh_data(io::IWriter& writer, const geometry::MeshData& mesh_data)
{
   write_header(writer, ResourceType::Mesh);

   MeshHeader mesh_header{};
   mesh_header.vertex_layout = MeshVertexLayout::NormalMapped;
   mesh_header.vertex_count = static_cast<u32>(mesh_data.vertex_data.vertices.size());
   mesh_header.index_count = static_cast<u32>(mesh_data.vertex_data.indices.size());
   mesh_header.group_count = static_cast<u32>(mesh_data.vertex_data.ranges.size());
   mesh_header.bounding_box_min = mesh_data.bounding_box.min;
   mesh_header.bounding_box_max = mesh_data.bounding_box.max;
   if (!writer.write({reinterpret_cast<const u8*>(&mesh_header), sizeof(MeshHeader)}).has_value()) {
      return false;
   }

   for (const auto& range : mesh_data.vertex_data.ranges) {
      auto name = ResourcePathMap::the().resolve(range.material_name);
      assert(name.size() != 0);

      if (!writer.write({reinterpret_cast<const u8*>(&range.offset), sizeof(MemorySize)}).has_value())
         return false;

      if (!writer.write({reinterpret_cast<const u8*>(&range.size), sizeof(MemorySize)}).has_value())
         return false;

      const auto name_size = static_cast<u32>(name.size());
      if (!writer.write({reinterpret_cast<const u8*>(&name_size), sizeof(u32)}).has_value())
         return false;

      if (!writer.write({reinterpret_cast<const u8*>(name.data()), name.size()}).has_value())
         return false;
   }

   if (!writer
           .write({reinterpret_cast<const u8*>(mesh_data.vertex_data.vertices.data()),
                   sizeof(geometry::Vertex) * mesh_data.vertex_data.vertices.size()})
           .has_value()) {
      return false;
   }

   if (!writer
           .write({reinterpret_cast<const u8*>(mesh_data.vertex_data.indices.data()), sizeof(u32) * mesh_data.vertex_data.indices.size()})
           .has_value()) {
      return false;
   }

   return true;
}

std::optional<geometry::MeshData> decode_mesh(io::IReader& reader, const u32 version)
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

   if (version <= 0x2500) {
      // until version 0.25.0 we just load the whole thing
      if (!reader
              .read({reinterpret_cast<u8*>(mesh_data.vertex_data.ranges.data()), sizeof(geometry::MaterialRange) * mesh_header.group_count})
              .has_value())
         return std::nullopt;
   } else {
      // from 0.25.1 resources are encoded by strings so we read each range
      // individually
      io::Deserializer decoder(reader);
      for (u32 i = 0; i < mesh_header.group_count; ++i) {
         const auto offset = decoder.read_MemorySize();
         const auto size = decoder.read_MemorySize();
         const auto rc_path_len = decoder.read_u32();
         assert(rc_path_len != 0);

         String path("\0"_rune, rc_path_len);
         if (auto res = reader.read({reinterpret_cast<u8*>(path.data()), rc_path_len}); !res.has_value())
            return std::nullopt;

         mesh_data.vertex_data.ranges[i] = geometry::MaterialRange{
            .offset = offset,
            .size = size,
            .material_name = name_from_path(path.view()),
         };
      }
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
