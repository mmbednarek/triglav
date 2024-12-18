#pragma once

#include "Loader.hpp"
#include "Resource.hpp"

#include "triglav/Name.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/Texture.hpp"
#include "triglav/io/Path.h"

#include <string_view>

namespace triglav::resource {

template<>
struct Loader<ResourceType::Texture>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Graphics};

   static graphics_api::Texture load_gpu(graphics_api::Device& device, TextureName name, const io::Path& path, const ResourceProperties& props);
};

}// namespace triglav::resource