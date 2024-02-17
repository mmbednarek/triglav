#pragma once

#include "Loader.hpp"

#include "graphics_api/Device.h"
#include "graphics_api/Shader.h"
#include "triglav/Name.hpp"

#include <string_view>

namespace triglav::resource {

template<>
struct resource::Loader<ResourceType::FragmentShader>
{
   constexpr static bool is_gpu_resource{true};

   static graphics_api::Shader load_gpu(graphics_api::Device &device, std::string_view path);
};

template<>
struct resource::Loader<ResourceType::VertexShader>
{
   constexpr static bool is_gpu_resource{true};

   static graphics_api::Shader load_gpu(graphics_api::Device &device, std::string_view path);
};

}// namespace triglav::resource
