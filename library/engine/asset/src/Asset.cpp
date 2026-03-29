#include "Asset.hpp"

#include "triglav/Int.hpp"
#include "triglav/Math.hpp"
#include "triglav/ResourcePathMap.hpp"
#include "triglav/ResourceType.hpp"
#include "triglav/String.hpp"
#include "triglav/io/Deserializer.hpp"
#include "triglav/io/DisplacedStream.hpp"
#include "triglav/json_util/Deserialize.hpp"
#include "triglav/json_util/Serialize.hpp"
#include "triglav/ktx/Texture.hpp"

#include <map>

namespace triglav::asset {

using namespace string_literals;
using namespace name_literals;

constexpr u32 g_magic_number = 0x53414754;
constexpr u32 g_last_version = 0x2502;

enum class MeshVertexLayout : u32
{
   Standard,    // Position: float3, Normal: float3, TexCoord: float2
   NormalMapped,// Position: float3, Normal: float3, TexCoord: float2, Tangent: float4
   SkeletalMesh,
};

struct LegacyVertex
{
   Vector3 location;
   Vector2 uv;
   Vector3 normal;
   Vector4 tangent;
};

struct LegacyMaterialRange
{
   MemorySize offset;
   MemorySize size;
   MaterialName material_name;
};

struct MeshHeader
{
   u32 vertex_buffer_size;
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
   mesh_header.vertex_buffer_size = static_cast<u32>(mesh_data.vertex_data.vertex_buffer.size());
   mesh_header.index_count = static_cast<u32>(mesh_data.vertex_data.index_buffer.size());
   mesh_header.group_count = static_cast<u32>(mesh_data.vertex_data.vertex_buffer.vertex_groups().size());
   mesh_header.bounding_box_min = mesh_data.bounding_box.min;
   mesh_header.bounding_box_max = mesh_data.bounding_box.max;
   if (!writer.write({reinterpret_cast<const u8*>(&mesh_header), sizeof(MeshHeader)}).has_value()) {
      return false;
   }

   for (const auto& range : mesh_data.vertex_data.vertex_buffer.vertex_groups()) {
      auto name = ResourcePathMap::the().resolve(range.material_name);
      assert(name.size() != 0);

      const auto vertex_size = geometry::get_vertex_size(range.components);
      assert(range.vertex_size % vertex_size == 0);
      const MemorySize vertex_count = range.vertex_size / vertex_size;

      if (!writer.write({reinterpret_cast<const u8*>(&range.components), sizeof(geometry::VertexComponentFlags)}).has_value())
         return false;

      if (!writer.write({reinterpret_cast<const u8*>(&vertex_count), sizeof(MemorySize)}).has_value())
         return false;

      if (!writer.write({reinterpret_cast<const u8*>(&range.index_offset), sizeof(MemorySize)}).has_value())
         return false;

      if (!writer.write({reinterpret_cast<const u8*>(&range.index_size), sizeof(MemorySize)}).has_value())
         return false;

      const auto name_size = static_cast<u32>(name.size());
      if (!writer.write({reinterpret_cast<const u8*>(&name_size), sizeof(u32)}).has_value())
         return false;

      if (!writer.write({reinterpret_cast<const u8*>(name.data()), name.size()}).has_value())
         return false;
   }

   if (!writer.write({mesh_data.vertex_data.vertex_buffer.data(), mesh_data.vertex_data.vertex_buffer.size()}).has_value()) {
      return false;
   }

   if (!writer
           .write({reinterpret_cast<const u8*>(mesh_data.vertex_data.index_buffer.data()),
                   sizeof(u32) * mesh_data.vertex_data.index_buffer.size()})
           .has_value()) {
      return false;
   }

   return true;
}

geometry::VertexComponentFlags component_flags_from_material(const MaterialName name)
{
   if (name == "material/bark.mat"_rc || name == "material/brick.mat"_rc || name == "material/gold.mat"_rc ||
       name == "material/grass.mat"_rc || name == "material/metal.mat"_rc || name == "material/pine.mat"_rc) {
      return geometry::VertexComponent::Core | geometry::VertexComponent::Texture | geometry::VertexComponent::NormalMap;
   }

   return geometry::VertexComponent::Core | geometry::VertexComponent::Texture;
}

std::optional<geometry::MeshData> decode_mesh(io::IReader& reader, const u32 version)
{
   if (version <= 0x2500) {
      // unsupported
      return std::nullopt;
   }

   [[maybe_unused]] u32 layout{};
   if (version <= 0x2501) {
      // Layout is per material range starting 0.25.02
      if (!reader.read({reinterpret_cast<u8*>(&layout), sizeof(MeshVertexLayout)}).has_value()) {
         return std::nullopt;
      }
   }

   MeshHeader mesh_header{};
   if (!reader.read({reinterpret_cast<u8*>(&mesh_header), sizeof(MeshHeader)}).has_value()) {
      return std::nullopt;
   }

   geometry::MeshData mesh_data{};
   mesh_data.bounding_box.min = mesh_header.bounding_box_min;
   mesh_data.bounding_box.max = mesh_header.bounding_box_max;

   // Starting 0.25.1, we encode resource names as strings, so we read each range
   // individually.
   if (version <= 0x2501) {
      std::vector<LegacyMaterialRange> material_ranges(mesh_header.group_count);

      io::Deserializer decoder(reader);
      for (u32 i = 0; i < mesh_header.group_count; ++i) {
         const auto offset = decoder.read_mem_size();
         const auto size = decoder.read_mem_size();
         const auto rc_path_len = decoder.read_u32();
         assert(rc_path_len != 0);

         String path("\0"_rune, rc_path_len);
         if (auto res = reader.read({reinterpret_cast<u8*>(path.data()), rc_path_len}); !res.has_value())
            return std::nullopt;

         material_ranges[i] = LegacyMaterialRange{
            .offset = offset,
            .size = size,
            .material_name = name_from_path(path.view()),
         };
      }

      std::vector<LegacyVertex> vertices;
      vertices.resize(mesh_header.vertex_buffer_size);
      if (!reader.read({reinterpret_cast<u8*>(vertices.data()), vertices.size() * sizeof(LegacyVertex)}).has_value()) {
         return std::nullopt;
      }

      std::vector<u32> indices(mesh_header.index_count);
      if (!reader.read({reinterpret_cast<u8*>(indices.data()), sizeof(u32) * mesh_header.index_count}).has_value()) {
         return std::nullopt;
      }

      mesh_data.vertex_data.index_buffer.resize(mesh_header.index_count);

      for (const auto& range : material_ranges) {
         u32 top_vertex_index = 0;
         std::map<u32, u32> unique_vertices{};

         for (u32 index = range.offset; index < range.offset + range.size; ++index) {
            const u32 vertex_index = indices[index];
            if (!unique_vertices.contains(vertex_index)) {
               unique_vertices[vertex_index] = top_vertex_index;
               ++top_vertex_index;
            }
            mesh_data.vertex_data.index_buffer[index] = unique_vertices[vertex_index];
         }

         const auto vertex_components = component_flags_from_material(range.material_name);
         const auto group_id = mesh_data.vertex_data.vertex_buffer.allocate_group(vertex_components, range.material_name,
                                                                                  unique_vertices.size(), range.offset, range.size);

         auto group = mesh_data.vertex_data.vertex_buffer.group(group_id);
         for (const auto [global_index, group_index] : unique_vertices) {
            const auto& vertex = vertices[global_index];
            group.get<geometry::VertexComponentCore>(group_index) = {vertex.location, vertex.normal};
            group.get<geometry::VertexComponentTexture>(group_index) = {vertex.uv};
            if (vertex_components & geometry::VertexComponent::NormalMap) {
               group.get<geometry::VertexComponentNormalMap>(group_index) = {vertex.tangent};
            }
         }
      }
   } else {
      io::Deserializer decoder(reader);
      for (u32 i = 0; i < mesh_header.group_count; ++i) {
         const auto vertex_components = geometry::VertexComponentFlags(decoder.read_u32());
         const auto vertex_count = decoder.read_mem_size();

         const auto index_offset = decoder.read_mem_size();
         const auto index_size = decoder.read_mem_size();

         const auto rc_path_len = decoder.read_u32();
         assert(rc_path_len != 0);

         String path("\0"_rune, rc_path_len);
         if (auto res = reader.read({reinterpret_cast<u8*>(path.data()), rc_path_len}); !res.has_value())
            return std::nullopt;

         mesh_data.vertex_data.vertex_buffer.allocate_group(vertex_components, name_from_path(path.view()), vertex_count, index_offset,
                                                            index_size);
      }

      assert(mesh_data.vertex_data.vertex_buffer.size() == mesh_header.vertex_buffer_size);

      if (!reader.read({mesh_data.vertex_data.vertex_buffer.data(), mesh_data.vertex_data.vertex_buffer.size()}).has_value()) {
         return std::nullopt;
      }

      mesh_data.vertex_data.index_buffer.resize(mesh_header.index_count);
      if (!reader.read({reinterpret_cast<u8*>(mesh_data.vertex_data.index_buffer.data()), sizeof(u32) * mesh_header.index_count})
              .has_value()) {
         return std::nullopt;
      }
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

bool encode_animation(io::IWriter& writer, Animation& animation)
{
   return json_util::serialize(animation.to_meta_ref(), writer, true);
}

std::optional<Animation> decode_animation(io::IReader& reader)
{
   Animation animation{};
   if (!json_util::deserialize(animation.to_meta_ref(), reader)) {
      return std::nullopt;
   }
   return animation;
}

}// namespace triglav::asset

#define TG_TYPE(NS) NS(NS(triglav, asset), AnimationChannelType)
TG_META_ENUM_BEGIN
TG_META_ENUM_VALUE(Translation)
TG_META_ENUM_VALUE(Rotation)
TG_META_ENUM_VALUE(Scale)
TG_META_ENUM_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, asset), AnimationChannel)
TG_META_CLASS_BEGIN
TG_META_PROPERTY(type, triglav::asset::AnimationChannelType)
TG_META_PROPERTY(channel_index, triglav::u32)
TG_META_ARRAY_PROPERTY(keyframes, triglav::Vector4)
TG_META_ARRAY_PROPERTY(timestamps, float)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, asset), Animation)
TG_META_CLASS_BEGIN
TG_META_ARRAY_PROPERTY(channels, triglav::asset::AnimationChannel)
TG_META_CLASS_END
#undef TG_TYPE
