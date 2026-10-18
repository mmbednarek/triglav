#include "MeshBVHStorage.hpp"

#include "triglav/asset/Asset.hpp"
#include "triglav/engine/Engine.hpp"
#include "triglav/project/PathManager.hpp"

namespace triglav::physics {

MeshBVHStorage& MeshBVHStorage::the()
{
   static MeshBVHStorage storage;
   return storage;
}

geometry::MeshBVH& MeshBVHStorage::get(const MeshName mesh_name)
{
   if (m_bvhs.contains(mesh_name))
      return m_bvhs.at(mesh_name);

   const auto rc_path = project::PathManager::the().translate_path(mesh_name);
   const auto mesh_data = asset::load_mesh_data(rc_path);

   geometry::MeshBVH bvh;
   bvh.build(mesh_data);

   auto [inserted_bvh, ok] = m_bvhs.emplace(mesh_name, std::move(bvh));
   assert(ok);
   return inserted_bvh->second;
}

BVHProvider::BVHProvider(world::Level& level) :
    m_level(level)
{
}

geometry::MeshBVH& BVHProvider::get_bottom_level(const world::EntityID entity_id) const
{
   const auto mesh_component = m_level.component<world::Mesh>(entity_id);
   return MeshBVHStorage::the().get(mesh_component.name);
}

}// namespace triglav::physics
