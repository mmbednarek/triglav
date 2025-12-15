#pragma once

#include "Loader.hpp"

#include "triglav/Name.hpp"
#include "triglav/io/Path.hpp"
#include "triglav/world/Level.hpp"

#include <set>
#include <string_view>

namespace triglav::resource {

template<>
struct Loader<ResourceType::Level>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Static};

   static world::Level load(const io::Path& path);
   static void collect_dependencies(std::set<ResourceName>& out_dependencies, const io::Path& path);
};

}// namespace triglav::resource
