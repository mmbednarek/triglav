#pragma once

#include "triglav/geometry/BVHTree.hpp"
#include "triglav/geometry/MeshData.hpp"
#include "triglav/world/World.hpp"

#include <map>

namespace triglav::physics {

struct MeshReference
{
   geometry::MeshData* mesh_data;

   [[nodiscard]] std::array<Vector3, 3> get_primitive(u32 index) const;
   std::uint32_t primitive_count() const;
};

class MeshBVHStorage
{
 public:
   geometry::BottomLevelBVH<MeshReference>& get(MeshName mesh_name);

   static MeshBVHStorage& the();

 private:
   std::map<MeshName, geometry::BottomLevelBVH<MeshReference>> m_bvhs{};
};

class BVHProvider
{
 public:
   explicit BVHProvider(world::Level& level);

   geometry::BottomLevelBVH<MeshReference>& get_bottom_level(world::EntityID entity_id) const;

 private:
   world::Level& m_level;
};

}// namespace triglav::physics
