#pragma once

#include "Loader.hpp"

#include "triglav/Name.hpp"
#include "triglav/io/Path.hpp"
#include "triglav/render_objects/Material.hpp"

namespace triglav::resource {

class ResourceManager;

template<>
struct Loader<ResourceType::Material>
{
   constexpr static ResourceLoadType type{ResourceLoadType::StaticDependent};

   static render_objects::Material load(ResourceManager& manager, const io::Path& path);
   static void collect_dependencies(std::vector<ResourceName>& out_dependencies, const io::Path& path);
};

}// namespace triglav::resource
