#pragma once

#include "Loader.hpp"

#include "triglav/ResourceType.hpp"
#include "triglav/world/Terrain.hpp"

namespace triglav::resource {

template<>
struct Loader<ResourceType::Terrain>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Static};

   static world::Terrain load(const io::Path& path);
   static void collect_dependencies(std::set<ResourceName>& out_dependencies, const io::Path& path);
};

}// namespace triglav::resource
