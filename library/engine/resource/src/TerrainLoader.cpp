#include "TerrainLoader.hpp"

#include "triglav/world/Terrain.hpp"

namespace triglav::resource {

using namespace name_literals;

world::Terrain Loader<ResourceType::Terrain>::load(const io::Path& path)
{
   auto terrain = world::Terrain::load_from_file(path);
   assert(terrain.has_value());
   return *terrain;
}

void Loader<ResourceType::Terrain>::collect_dependencies(std::set<ResourceName>& /*out_dependencies*/, const io::Path& /*path*/) {}

}// namespace triglav::resource
