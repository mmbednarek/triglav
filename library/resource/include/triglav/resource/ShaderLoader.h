#pragma once

#include "Loader.hpp"

#include "triglav/graphics_api/Device.h"
#include "triglav/graphics_api/Shader.h"
#include "triglav/io/Path.h"
#include "triglav/Name.hpp"

#include <string_view>

namespace triglav::resource {

template<>
struct Loader<ResourceType::FragmentShader>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Graphics};

   static graphics_api::Shader load_gpu(graphics_api::Device &device, const io::Path& path);
};

template<>
struct Loader<ResourceType::VertexShader>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Graphics};

   static graphics_api::Shader load_gpu(graphics_api::Device &device, const io::Path& path);
};

}// namespace triglav::resource
