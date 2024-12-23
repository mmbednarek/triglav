#pragma once

#include "Loader.hpp"

#include "triglav/Name.hpp"
#include "triglav/io/Path.hpp"
#include "triglav/world/Level.hpp"

#include <string_view>

namespace triglav::resource {

template<>
struct Loader<ResourceType::Level>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Static};

   static world::Level load(const io::Path& path);
};

}// namespace triglav::resource
