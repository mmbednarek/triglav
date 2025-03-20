#pragma once

#include "triglav/asset/Asset.hpp"
#include "triglav/io/Path.hpp"

#include <optional>

namespace triglav::tool::cli {

struct TextureImportProps
{
   io::Path srcPath;
   io::Path dstPath;
   asset::TexturePurpose purpose;
   asset::SamplerProperties samplerProperties;
   bool shouldCompress{};
   bool hasMipMaps{};
   bool shouldOverride{};
};

struct ImageData
{
   std::vector<uint32_t> imageData;
   Vector2u size;
};

[[nodiscard]] std::optional<ImageData> load_image_data(io::ISeekableStream& stream);
[[nodiscard]] bool import_texture_from_stream(const TextureImportProps& props, io::ISeekableStream& stream);
[[nodiscard]] bool import_texture(const TextureImportProps& props);

}// namespace triglav::tool::cli
