#pragma once

#include "Loader.hpp"

#include "triglav/graphics_api/Device.h"
#include "triglav/graphics_api/Texture.h"
#include "triglav/Name.hpp"

#include <string_view>

namespace triglav::resource {

template<>
struct Loader<ResourceType::Texture>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Graphics};

   static graphics_api::Texture load_gpu(graphics_api::Device &device, std::string_view path);
};

}// namespace triglav::resource