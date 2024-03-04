#pragma once

#include "Loader.hpp"

#include "triglav/Name.hpp"
#include "triglav/render_core/Material.hpp"

#include <string_view>

namespace triglav::resource {

template<>
struct Loader<ResourceType::Material>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Static};

   static render_core::Material load(std::string_view path);
};

}// namespace triglav::resource
