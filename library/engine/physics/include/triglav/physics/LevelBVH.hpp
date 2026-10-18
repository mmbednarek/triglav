#pragma once

#include "triglav/Macros.hpp"
#include "triglav/geometry/Geometry.hpp"
#include "triglav/world/World.hpp"

#include <map>

namespace triglav::physics {

class BVHProvider;

class LevelBVH
{
 public:
   struct Node
   {
      Node* parent{};
      geometry::BoundingBox bounding_box{};
      bool is_leaf_node = true;

      union
      {
         struct
         {
            float max_surface_area{};
            Node* left{};
            Node* right{};
         } internal{};
         struct
         {
            world::EntityID entity_id;
            Matrix4x4 inv_transform;
         } leaf;
      };

      [[nodiscard]] bool is_leaf() const
      {
         return this->is_leaf_node;
      }
   };

   struct RayHit
   {
      float distance = INF;
      world::EntityID entity_id = world::NO_ENTITY;

      static RayHit miss()
      {
         return {};
      }

      [[nodiscard]] float score() const
      {
         return distance;
      }
   };

   struct SweepHit
   {
      float time = INF;
      Vector3 normal{};
      world::EntityID entity_id = world::NO_ENTITY;

      static SweepHit miss()
      {
         return {};
      }

      [[nodiscard]] float score() const
      {
         return time;
      }
   };

   using NodeMappingContainer = std::map<world::EntityID, Node*>;

   explicit LevelBVH(world::Level& level, BVHProvider& provider);
   ~LevelBVH();

   TG_DELETE_COPY(LevelBVH);

   LevelBVH(LevelBVH&& other) noexcept;
   LevelBVH& operator=(LevelBVH&& other) noexcept;

   void add(world::EntityID entity_id);
   void remove(world::EntityID entity_id);
   void update(world::EntityID entity_id);
   void build();
   void clear();
   [[nodiscard]] RayHit ray_cast(const geometry::Ray& ray) const;
   [[nodiscard]] SweepHit sweep_cast(const geometry::BoundingBox& bb, const Vector3& movement) const;

 private:
   world::Level& m_level;
   BVHProvider& m_mesh_provider;

   Node* m_root{};
   NodeMappingContainer m_leave_mapping;
};

}// namespace triglav::physics
