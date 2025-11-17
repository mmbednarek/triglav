#pragma once

#include "triglav/asset/Asset.hpp"
#include "triglav/io/Path.hpp"

#include <optional>

namespace triglav::tool::cli {

struct TextureImportProps
{
   io::Path src_path;
   io::Path dst_path;
   asset::TexturePurpose purpose;
   asset::SamplerProperties sampler_properties;
   bool should_compress{};
   bool has_mip_maps{};
   bool should_override{};
};

struct ImageData
{
   std::vector<uint32_t> image_data;
   Vector2u size;
};

[[nodiscard]] std::optional<ImageData> load_image_data(io::ISeekableStream& stream);
[[nodiscard]] bool import_texture_from_stream(const TextureImportProps& props, io::ISeekableStream& stream);
[[nodiscard]] bool import_texture(const TextureImportProps& props);

}// namespace triglav::tool::cli
