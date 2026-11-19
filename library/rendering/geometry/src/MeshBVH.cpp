#include "MeshBVH.hpp"

#include "BVHUtil.hpp"

namespace triglav::geometry {

namespace {

BoundingBox bounding_box_from_triangle(const std::array<Vector3, 3>& primitive)
{
   BoundingBox result{
      .min = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()},
      .max = {std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()},
   };
   for (u32 i = 0; i < 3; ++i) {
      result.min = {std::min(result.min.x, primitive[i].x), std::min(result.min.y, primitive[i].y), std::min(result.min.z, primitive[i].z)};
      result.max = {std::max(result.max.x, primitive[i].x), std::max(result.max.y, primitive[i].y), std::max(result.max.z, primitive[i].z)};
   }
   return result;
}


std::array<Vector3, 3> get_mesh_primitive(const MeshData& mesh_data, const PrimitiveID index)
{
   return {
      mesh_data.vertex_position_at(3 * index + 0),
      mesh_data.vertex_position_at(3 * index + 1),
      mesh_data.vertex_position_at(3 * index + 2),
   };
}

struct MeshBVHBaseTraits
{
   using Node = MeshBVH::Node;
   using UserData = std::monostate;

   static Node* move_payload(const UserData* /*user_data*/, Node* leaf_node)
   {
      auto* out = new Node{
         .is_leaf_node = true,
         .bounding_box = leaf_node->bounding_box,
         .leaf =
            {
               .primitive_id = leaf_node->leaf.primitive_id,
               .primitive = leaf_node->leaf.primitive,
            },
      };

      leaf_node->is_leaf_node = false;
      leaf_node->internal.left = nullptr;
      leaf_node->internal.right = nullptr;
      return out;
   }

   static float calculate_insert_cost(UserData* /*user_data*/, const Node* parent, const Node* new_child)
   {
      return parent->bounding_box.extension_cost(new_child->bounding_box);
   }

   static void recalculate_bounds(const UserData* /*user_data*/, Node* parent)
   {
      parent->bounding_box = parent->internal.left->bounding_box.merge(parent->internal.right->bounding_box);
   }
};

using MeshBVHUtil = BVHUtil<MeshBVHBaseTraits>;

struct MeshBVHRayTraversalTraits
{
   using BaseTraits = MeshBVHBaseTraits;
   using Node = BaseTraits::Node;
   using Result = MeshBVH::RayHit;
   using Traversary = Ray;
   using IntersectionResult = BoundingBox::RayHit;
   using UserData = std::monostate;

   static Result test_leaf(const UserData* /*user_data*/, const Node* node, const Ray& ray, float& best_score)
   {
      assert(node->is_leaf_node);
      const auto distance = ray.intersect_triangle(node->leaf.primitive);
      if (!distance.has_value() || *distance > best_score)
         return Result::miss();

      best_score = *distance;
      return {*distance, node->leaf.primitive_id};
   }

   static IntersectionResult test_intersection(const UserData* /*user_data*/, const Node* node, const Ray& ray)
   {
      return node->bounding_box.intersect(ray);
   }

   static float test_hit_score(const UserData* /*user_data*/, const IntersectionResult& inter_result, const Node* /*node*/,
                               const Ray& /*traversary*/)
   {
      assert(inter_result.has_hit());
      return inter_result.min_distance;
   }
};

using MeshBVHRayTraverser = BVHTraverser<MeshBVHRayTraversalTraits>;

struct MeshBVHSweepTraversalTraits
{
   using BaseTraits = MeshBVHBaseTraits;
   using Node = BaseTraits::Node;
   using Result = MeshBVH::SweepHit;
   using UserData = std::monostate;
   struct Traversary
   {
      BoundingBox bounding_box;
      Vector3 movement;
   };
   struct IntersectionResult
   {
      bool does_intersect = false;
      [[nodiscard]] bool has_hit() const
      {
         return does_intersect;
      }
   };

   static Result test_leaf(const UserData* /*user_data*/, const Node* node, const Traversary& traversary, float& best_score)
   {
      const auto sweep_result = sweep_triangle_aabb(node->leaf.primitive, traversary.bounding_box, traversary.movement);
      if (sweep_result.time >= best_score) {
         return Result::miss();
      }

      best_score = sweep_result.time;

      return {
         .time = sweep_result.time,
         .normal = sweep_result.normal,
         .primitive_id = node->leaf.primitive_id,
      };
   }

   static IntersectionResult test_intersection(const UserData* /*user_data*/, const Node* node, const Traversary& traversary)
   {
      return {node->bounding_box.does_intersect_aabb(traversary.bounding_box.extend_by_movement(traversary.movement))};
   }

   static float test_hit_score(const UserData* /*user_data*/, const IntersectionResult& /*inter_result*/, const Node* node,
                               const Traversary& traversary)
   {
      return node->bounding_box.sweep_aabb(traversary.bounding_box, traversary.movement).entry_time;
   }
};

using MeshBVHSweepTraverser = BVHTraverser<MeshBVHSweepTraversalTraits>;

struct MeshBVHCapsuleSweepTraversalTraits
{
   using BaseTraits = MeshBVHBaseTraits;
   using Node = BaseTraits::Node;
   using Result = MeshBVH::SweepHit;
   using UserData = std::monostate;
   struct Traversary
   {
      Capsule capsule;
      Vector3 movement;
   };
   struct IntersectionResult
   {
      bool does_intersect = false;
      [[nodiscard]] bool has_hit() const
      {
         return does_intersect;
      }
   };

   static Result test_leaf(const UserData* /*user_data*/, const Node* node, const Traversary& traversary, float& best_score)
   {
      const auto sweep_result = sweep_triangle_capsule(node->leaf.primitive, traversary.capsule, traversary.movement);
      if (sweep_result.time >= best_score) {
         return Result::miss();
      }

      best_score = sweep_result.time;

      return {
         .time = sweep_result.time,
         .normal = sweep_result.normal,
         .primitive_id = node->leaf.primitive_id,
      };
   }

   static IntersectionResult test_intersection(const UserData* /*user_data*/, const Node* node, const Traversary& traversary)
   {
      return {node->bounding_box.does_intersect_capsule(traversary.capsule.extend_by_movement(traversary.movement))};
   }

   static float test_hit_score(const UserData* /*user_data*/, const IntersectionResult& /*inter_result*/, const Node* node,
                               const Traversary& traversary)
   {
      return node->bounding_box.sweep_capsule(traversary.capsule, traversary.movement).entry_time;
   }
};

using MeshBVHCapsuleSweepTraverser = BVHTraverser<MeshBVHCapsuleSweepTraversalTraits>;

}// namespace

MeshBVH::~MeshBVH()
{
   this->clear();
}

MeshBVH::MeshBVH(MeshBVH&& other) noexcept :
    m_root(std::exchange(other.m_root, nullptr))
{
}

MeshBVH& MeshBVH::operator=(MeshBVH&& other) noexcept
{
   this->clear();
   m_root = std::exchange(other.m_root, nullptr);
   return *this;
}

void MeshBVH::build(const MeshData& mesh_data)
{
   this->clear();

   const auto primitive_count = mesh_data.index_count() / 3;
   if (primitive_count == 0)
      return;

   const auto base_primitive = get_mesh_primitive(mesh_data, 0);
   m_root = new Node{
      .is_leaf_node = true,
      .bounding_box = bounding_box_from_triangle(base_primitive),
      .leaf =
         {
            .primitive_id = 0,
            .primitive = base_primitive,
         },
   };
   for (u32 i = 1; i < primitive_count; ++i) {
      auto primitive = get_mesh_primitive(mesh_data, i);
      MeshBVHUtil::insert_node(nullptr, m_root,
                               new Node{
                                  .is_leaf_node = true,
                                  .bounding_box = bounding_box_from_triangle(primitive),
                                  .leaf =
                                     {
                                        .primitive_id = i,
                                        .primitive = primitive,
                                     },
                               });
   }
}

MeshBVH::RayHit MeshBVH::ray_cast(const Ray& ray) const
{
   if (m_root == nullptr)
      return RayHit::miss();

   if (!m_root->bounding_box.intersect(ray).has_hit())
      return RayHit::miss();

   Ray mut_ray = ray;
   return MeshBVHRayTraverser::traverse(nullptr, m_root, mut_ray, mut_ray.distance);
}

MeshBVH::SweepHit MeshBVH::sweep_cast(const BoundingBox& bb, const Vector3& movement) const
{
   float best_score = INF;
   return MeshBVHSweepTraverser::traverse(nullptr, m_root, {bb, movement}, best_score);
}

MeshBVH::SweepHit MeshBVH::sweep_cast_capsule(const Capsule& capsule, const Vector3& movement) const
{
   float best_score = INF;
   return MeshBVHCapsuleSweepTraverser::traverse(nullptr, m_root, {capsule, movement}, best_score);
}

geometry::SweepHit MeshBVH::generic_sweep_cast(const BoundingBox& box, const Vector3& movement) const
{
   const auto res = this->sweep_cast(box, movement);
   return {res.time, res.normal};
}

float MeshBVH::generic_ray_cast(const Ray& ray) const
{
   const auto res = this->ray_cast(ray);
   return res.distance;
}

void MeshBVH::clear()
{
   if (m_root == nullptr)
      return;

   MeshBVHUtil::destroy(m_root);
   m_root = nullptr;
}

}// namespace triglav::geometry
