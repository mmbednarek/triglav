#include "LevelLoader.hpp"

#include "triglav/ResourcePathMap.hpp"
#include "triglav/io/File.hpp"

#include <ryml.hpp>
#include <string>

namespace triglav::resource {

world::Level Loader<ResourceType::Level>::load(const io::Path& /*path*/)
{
   throw std::runtime_error("DEPRECATED FEATURE");
   return {};
}

void Loader<ResourceType::Level>::collect_dependencies(std::set<ResourceName>& /*out_dependencies*/, const io::Path& /*path*/)
{
   throw std::runtime_error("DEPRECATED FEATURE");
}

static_assert(CollectsDependencies<Loader<ResourceType::Level>>);

}// namespace triglav::resource
