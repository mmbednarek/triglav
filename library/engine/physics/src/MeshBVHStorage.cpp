#include "MeshBVHStorage.hpp"

#include "triglav/asset/Asset.hpp"
#include "triglav/engine/Engine.hpp"
#include "triglav/project/PathManager.hpp"

namespace triglav::physics {

std::array<Vector3, 3> MeshReference::get_primitive(const u32 index) const
{
   return {
      mesh_data->vertex_position_at(3 * index + 0),
      mesh_data->vertex_position_at(3 * index + 1),
      mesh_data->vertex_position_at(3 * index + 2),
   };
}

u32 MeshReference::primitive_count() const
{
   return mesh_data->primitive_count() / 3;
}

MeshBVHStorage& MeshBVHStorage::the()
{
   static MeshBVHStorage storage;
   return storage;
}

geometry::BottomLevelBVH<MeshReference>& MeshBVHStorage::get(const MeshName mesh_name)
{
   if (m_bvhs.contains(mesh_name))
      return m_bvhs.at(mesh_name);

   const auto rc_path = project::PathManager::the().translate_path(mesh_name);
   auto mesh_data = asset::load_mesh_data(rc_path);
   MeshReference mesh_ref{&mesh_data};

   geometry::BottomLevelBVH<MeshReference> bvh;
   bvh.build(mesh_ref);

   auto [inserted_bvh, ok] = m_bvhs.emplace(mesh_name, std::move(bvh));
   assert(ok);
   return inserted_bvh->second;
}

BVHProvider::BVHProvider(world::Level& level) :
    m_level(level)
{
}

geometry::BottomLevelBVH<MeshReference>& BVHProvider::get_bottom_level(const world::EntityID entity_id) const
{
   const auto mesh_component = m_level.component<world::Mesh>(entity_id);
   return MeshBVHStorage::the().get(mesh_component.name);
}

}// namespace triglav::physics
