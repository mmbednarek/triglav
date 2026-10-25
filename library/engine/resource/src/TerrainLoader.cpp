#include "TerrainLoader.hpp"

#include "triglav/asset/Asset.hpp"

namespace triglav::resource {

using namespace name_literals;

geometry::Terrain Loader<ResourceType::Terrain>::load(const io::Path& path)
{
   auto file = io::open_file(path, io::FileMode::Read);
   auto terrain = asset::decode_terrain(**file);
   assert(terrain.has_value());
   return *terrain;
}

void Loader<ResourceType::Terrain>::collect_dependencies(std::set<ResourceName>& /*out_dependencies*/, const io::Path& /*path*/) {}

}// namespace triglav::resource
