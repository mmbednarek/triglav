#pragma once

#include "Loader.hpp"
#include "ResourceManager.hpp"

#include "triglav/Name.hpp"
#include "triglav/io/Path.hpp"
#include "triglav/render_objects/Material.hpp"

#include <string_view>

namespace triglav::resource {

template<>
struct Loader<ResourceType::Material>
{
   constexpr static ResourceLoadType type{ResourceLoadType::StaticDependent};

   static render_objects::Material load(ResourceManager& manager, const io::Path& path);
};

}// namespace triglav::resource
