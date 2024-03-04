#pragma once

#include "Loader.hpp"

#include "triglav/Name.hpp"
#include "triglav/world/Level.h"

#include <string_view>

namespace triglav::resource {

template<>
struct Loader<ResourceType::Level>
{
  constexpr static ResourceLoadType type{ResourceLoadType::Static};

   static world::Level load(std::string_view path);
};

}// namespace triglav::resource
