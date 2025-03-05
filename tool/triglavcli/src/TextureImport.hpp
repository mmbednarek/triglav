#pragma once

#include "triglav/io/Path.hpp"
#include "triglav/asset/Asset.hpp"

namespace triglav::tool::cli {

struct TextureImportProps
{
   io::Path srcPath;
   io::Path dstPath;
   asset::TexturePurpose purpose;
   asset::SamplerProperties samplerProperties;
   bool shouldCompress{};
   bool hasMipMaps{};
};

[[nodiscard]] bool import_texture(const TextureImportProps& props);

}
