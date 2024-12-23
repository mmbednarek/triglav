#pragma once

#include "Loader.hpp"
#include "Resource.hpp"

#include "triglav/Name.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/Shader.hpp"
#include "triglav/io/Path.hpp"

#include <string_view>

namespace triglav::resource {

template<>
struct Loader<ResourceType::FragmentShader>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Graphics};

   static graphics_api::Shader load_gpu(graphics_api::Device& device, FragmentShaderName name, const io::Path& path,
                                        const ResourceProperties& props);
};

template<>
struct Loader<ResourceType::VertexShader>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Graphics};

   static graphics_api::Shader load_gpu(graphics_api::Device& device, VertexShaderName name, const io::Path& path,
                                        const ResourceProperties& props);
};

template<>
struct Loader<ResourceType::ComputeShader>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Graphics};

   static graphics_api::Shader load_gpu(graphics_api::Device& device, ComputeShaderName name, const io::Path& path,
                                        const ResourceProperties& props);
};

template<>
struct Loader<ResourceType::RayGenShader>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Graphics};

   static graphics_api::Shader load_gpu(graphics_api::Device& device, RayGenShaderName name, const io::Path& path,
                                        const ResourceProperties& props);
};

template<>
struct Loader<ResourceType::RayClosestHitShader>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Graphics};

   static graphics_api::Shader load_gpu(graphics_api::Device& device, RayClosestHitShaderName name, const io::Path& path,
                                        const ResourceProperties& props);
};

template<>
struct Loader<ResourceType::RayMissShader>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Graphics};

   static graphics_api::Shader load_gpu(graphics_api::Device& device, RayMissShaderName name, const io::Path& path,
                                        const ResourceProperties& props);
};

}// namespace triglav::resource
