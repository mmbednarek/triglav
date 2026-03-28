#pragma once

#include "Loader.hpp"

#include "triglav/ResourceType.hpp"
#include "triglav/render_objects/Armature.hpp"

namespace triglav::resource {

template<>
struct Loader<ResourceType::Armature>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Static};

   static render_objects::Armature load(const io::Path& path);
   static void collect_dependencies(std::set<ResourceName>& out_dependencies, const io::Path& path);
};

}// namespace triglav::resource
