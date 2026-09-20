#include "TerrainLoader.hpp"

#include "triglav/world/Terrain.hpp"

namespace triglav::resource {

using namespace name_literals;

world::Terrain Loader<ResourceType::Terrain>::load(const io::Path& /*path*/)
{
   return world::Terrain({1024, 1024}, "engine/texture/grass.tex"_rc);
}

void Loader<ResourceType::Terrain>::collect_dependencies(std::set<ResourceName>& /*out_dependencies*/, const io::Path& /*path*/) {}

}// namespace triglav::resource
