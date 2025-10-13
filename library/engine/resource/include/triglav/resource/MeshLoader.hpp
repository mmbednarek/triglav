#pragma once

#include "Loader.hpp"
#include "Resource.hpp"

#include "triglav/Name.hpp"
#include "triglav/io/Path.hpp"
#include "triglav/render_objects/Mesh.hpp"

#include <string_view>

namespace triglav::resource {

template<>
struct Loader<ResourceType::Mesh>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Graphics};

   static render_objects::Mesh load_gpu(graphics_api::Device& device, MeshName name, const io::Path& path, const ResourceProperties& props);
};

}// namespace triglav::resource
