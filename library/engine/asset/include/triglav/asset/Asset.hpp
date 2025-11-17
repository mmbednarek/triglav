#pragma once

#include "triglav/geometry/Mesh.hpp"
#include "triglav/io/File.hpp"
#include "triglav/io/Stream.hpp"

#include <optional>
#include <string_view>

namespace triglav::ktx {
class Texture;
}

namespace triglav::asset {

struct AssetHeader
{
   u32 magic;// 0x53414754
   u32 version;
   ResourceType type;
};

enum class FilterType : u32
{
   Linear = 0b0,
   NearestNeighbour = 0b1,
};

inline std::optional<FilterType> filter_type_from_string(const std::string_view str)
{
   if (str == "linear")
      return FilterType::Linear;
   if (str == "nearest")
      return FilterType::NearestNeighbour;
   return std::nullopt;
}

inline std::string_view filter_type_to_string(const FilterType filter_type)
{
   switch (filter_type) {
   case FilterType::Linear:
      return "linear";
   case FilterType::NearestNeighbour:
      return "nearest";
   }
   return "";
}

enum class TextureAddressMode : u32
{
   Repeat = 0b000,
   Mirror = 0b001,
   Clamp = 0b010,
   Border = 0b011,
   MirrorOnce = 0b100,
};

inline std::optional<TextureAddressMode> texture_address_mode_from_string(const std::string_view str)
{
   if (str == "repeat")
      return TextureAddressMode::Repeat;
   if (str == "mirror")
      return TextureAddressMode::Mirror;
   if (str == "clamp")
      return TextureAddressMode::Clamp;
   if (str == "border")
      return TextureAddressMode::Border;
   if (str == "mirroronce")
      return TextureAddressMode::MirrorOnce;
   return std::nullopt;
}

inline std::string_view texture_address_mode_to_string(const TextureAddressMode address_mode)
{
   switch (address_mode) {
   case TextureAddressMode::Repeat:
      return "repeat";
   case TextureAddressMode::Mirror:
      return "mirror";
   case TextureAddressMode::Clamp:
      return "clamp";
   case TextureAddressMode::Border:
      return "border";
   case TextureAddressMode::MirrorOnce:
      return "mirroronce";
   }
   return "";
}

using EncodedSamplerProperties = u32;

struct SamplerProperties
{
   FilterType min_filter{};
   FilterType mag_filter{};
   TextureAddressMode address_mode_u{};
   TextureAddressMode address_mode_v{};
   TextureAddressMode address_mode_w{};
   bool enable_anisotropy{};
};

enum class TextureFormat : u8
{
   KTX_Uncompressed = 0b0,
   KTX_CompressedBC3 = 0b1,
};

enum class TexturePurpose : u8
{
   Albedo = 0,
   AlbedoWithAlpha = 1,
   NormalMap = 2,
   BumpMap = 3,
   Metallic = 4,
   Roughness = 5,
   Mask = 6,
};

struct TextureHeader
{
   TextureFormat format;
   TexturePurpose purpose;
   MemorySize payload_size;
   EncodedSamplerProperties sampler_properties;
};

struct DecodedTexture
{
   ktx::Texture texture;
   TextureFormat format;
   TexturePurpose purpose;
   SamplerProperties sampler_props;
};

EncodedSamplerProperties encode_sampler_properties(const SamplerProperties& properties);
SamplerProperties decode_sampler_properties(EncodedSamplerProperties encoded_properties);

std::optional<AssetHeader> decode_header(io::IReader& reader);

bool encode_mesh(io::IWriter& writer, const geometry::Mesh& mesh);
std::optional<geometry::MeshData> decode_mesh(io::IReader& reader);

bool encode_texture(io::IWriter& writer, TexturePurpose purpose, const ktx::Texture& tex, const SamplerProperties& sampler);
std::optional<DecodedTexture> decode_texture(io::IFile& stream);

}// namespace triglav::asset