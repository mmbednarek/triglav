#pragma once

#include "Collider.hpp"

#include "triglav/geometry/MeshBVH.hpp"
#include "triglav/world/World.hpp"

#include <map>

namespace triglav::physics {

class MeshBVHStorage
{
 public:
   geometry::MeshBVH& get(MeshName mesh_name);

   static MeshBVHStorage& the();

 private:
   std::map<MeshName, geometry::MeshBVH> m_bvhs{};
};

class BVHProvider
{
 public:
   explicit BVHProvider(world::Level& level);

   [[nodiscard]] const ColliderRef* get_bottom_level(world::EntityID entity_id) const;

 private:
   world::Level& m_level;
   mutable std::map<world::EntityID, ColliderRef> m_cache;
};

}// namespace triglav::physics
