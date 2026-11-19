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

const TraversableObject* BVHProvider::get_bottom_level(const world::EntityID entity_id) const
{
   const auto it = m_cache.find(entity_id);
   if (it != m_cache.end()) {
      return &it->second;
   }

   const auto& transform = m_level.component_opt<Transform3D>(entity_id, Transform3D::identity());

   if (const auto* mesh_component = m_level.component_opt<world::Mesh>(entity_id); mesh_component != nullptr) {
      const auto& bvh_mesh = MeshBVHStorage::the().get(mesh_component->name);
      auto [emplace_it, ok] = m_cache.emplace(entity_id, TraversableObject{
                                                            .kind = TraversableKind::Mesh,
                                                            .inverse_transform = transform.to_matrix().inverse(),
                                                            .payload = &bvh_mesh,
                                                         });
      assert(ok);
      return &emplace_it->second;
   }
   if (const auto* terrain_component = m_level.component_opt<world::TerrainComponent>(entity_id); terrain_component != nullptr) {
      const auto* terrain = &engine::resource_manager().get(terrain_component->name);
      auto [emplace_it, ok] = m_cache.emplace(entity_id, TraversableObject{
                                                            .kind = TraversableKind::Terrain,
                                                            .inverse_transform = transform.to_matrix().inverse(),
                                                            .payload = terrain,
                                                         });
      assert(ok);
      return &emplace_it->second;
   }
   return nullptr;
}

}// namespace triglav::physics
