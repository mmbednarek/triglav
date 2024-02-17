#pragma once

#include "Loader.hpp"

#include "graphics_api/Texture.h"
#include "triglav/Name.hpp"

#include <string_view>

namespace triglav::resource {

template<>
struct Loader<ResourceType::Mesh>
{
   constexpr static bool is_gpu_resource{true};

   static graphics_api::Texture load_gpu(graphics_api::Device &device, std::string_view path);
};

}// namespace triglav::resource
