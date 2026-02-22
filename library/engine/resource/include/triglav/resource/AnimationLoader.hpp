#pragma once

#include "Loader.hpp"
#include "triglav/ResourceType.hpp"

namespace triglav::resource {

template<>
struct Loader<ResourceType::Animation>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Static};

   static asset::Animation load(const io::Path& path);
   static void collect_dependencies(std::set<ResourceName>& out_dependencies, const io::Path& path);
};

}// namespace triglav::resource
