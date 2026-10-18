#pragma once

#include "triglav/Macros.hpp"

#include "Geometry.hpp"
#include "MeshData.hpp"

namespace triglav::geometry {

using PrimitiveID = u32;
constexpr PrimitiveID NO_PRIMITIVE_ID = ~0u;

class MeshBVH
{
 public:
   struct Node
   {
      bool is_leaf_node{true};
      BoundingBox bounding_box{};
      union
      {
         struct
         {
            Node* left{};
            Node* right{};
         } internal{};
         struct
         {
            u32 primitive_id;
            std::array<Vector3, 3> primitive;
         } leaf;
      };

      [[nodiscard]] bool is_leaf() const
      {
         return is_leaf_node;
      }
   };

   struct RayHit
   {
      float distance;
      u32 primitive_id;

      [[nodiscard]] static RayHit miss()
      {
         return RayHit{
            .distance = INF,
            .primitive_id = NO_PRIMITIVE_ID,
         };
      }

      [[nodiscard]] float score() const
      {
         return distance;
      }
   };

   struct SweepHit
   {
      float time{INF};
      Vector3 normal{};
      u32 primitive_id{~0u};

      static SweepHit miss()
      {
         return SweepHit{};
      }

      [[nodiscard]] float score() const
      {
         return time;
      }
   };

   MeshBVH() = default;
   ~MeshBVH();

   TG_DELETE_COPY(MeshBVH)

   MeshBVH(MeshBVH&& other) noexcept;
   MeshBVH& operator=(MeshBVH&& other) noexcept;

   void build(const MeshData& mesh_data);

   [[nodiscard]] RayHit ray_cast(const Ray& ray) const;
   [[nodiscard]] SweepHit sweep_cast(const BoundingBox& bb, const Vector3& movement) const;

   void clear();

 private:
   Node* m_root{};
};

}// namespace triglav::geometry
