#pragma once

#include "Loader.hpp"
#include "ResourceManager.hpp"

#include "triglav/Name.hpp"
#include "triglav/io/Path.hpp"
#include "triglav/render_core/Material.hpp"

#include <string_view>

namespace triglav::resource {

template<>
struct Loader<ResourceType::MaterialTemplate>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Static};

   static render_core::MaterialTemplate load(const io::Path& path);
};

template<>
struct Loader<ResourceType::Material>
{
   constexpr static ResourceLoadType type{ResourceLoadType::StaticDependent};

   static render_core::Material load(ResourceManager& manager, const io::Path& path);
};

}// namespace triglav::resource
