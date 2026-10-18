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

struct TriangleSweepResult
{
   bool hit_found;
   float time;
   Vector3 normal;
};

[[nodiscard]] TriangleSweepResult sweep_aabb_triangle(const BoundingBox& bb, const Vector3& movement,
                                                      const std::array<Vector3, 3>& triangle)
{
   constexpr static float TOLERANCE = 1e-8f;

   // Work in box space: box centered at origin, triangle static.
   const auto box_center = bb.centroid();
   const Vector3 v[3] = {triangle[0] - box_center, triangle[1] - box_center, triangle[2] - box_center};
   const Vector3 h = bb.half_extent();

   const Vector3 e[3] = {v[1] - v[0], v[2] - v[1], v[0] - v[2]};
   static constexpr std::array box_axes = {Vector3{1, 0, 0}, Vector3{0, 1, 0}, Vector3{0, 0, 1}};

   // Build the 13 candidate axes.
   Vector3 axes[13];
   int n_axes = 0;
   for (int i = 0; i < 3; ++i)
      axes[n_axes++] = box_axes[i];
   axes[n_axes++] = e[0].cross(e[1]);// triangle normal
   for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j)
         axes[n_axes++] = box_axes[i].cross(e[j]);// edge x edge

   float t_enter = -INF;// latest entry over all axes
   float t_exit = INF;  // earliest exit over all axes
   Vector3 enter_normal{};

   // Fallback for initial overlap: axis with smallest penetration.
   float best_depth = INF;
   Vector3 mtv_normal{};

   for (int i = 0; i < n_axes; ++i) {
      if (axes[i].length_sq() < TOLERANCE)// degenerate (parallel edges, ...)
         continue;
      const Vector3 L = axes[i].normalize();

      // Box radius on L, triangle interval on L, box velocity on L.
      const float r = h.x * std::fabs(L.x) + h.y * std::fabs(L.y) + h.z * std::fabs(L.z);
      const float d0 = v[0].dot(L), d1 = v[1].dot(L), d2 = v[2].dot(L);
      const float tmin = std::min({d0, d1, d2});
      const float tmax = std::max({d0, d1, d2});
      const float s = movement.dot(L);

      // Initial penetration depth on this axis (push box toward -L or +L).
      const float push_neg = r - tmin;// box_max - tri_min
      const float push_pos = tmax + r;// tri_max - box_min
      if (push_neg < 0.0f || push_pos < 0.0f) {
         // Separated at t = 0 on this axis.
      } else {
         const float depth = std::min(push_neg, push_pos);
         if (depth < best_depth) {
            best_depth = depth;
            mtv_normal = (push_neg < push_pos) ? L * -1.0f : L;
         }
      }

      if (std::fabs(s) < TOLERANCE) {
         // No relative motion on this axis: permanently separated or overlapping.
         if (tmin > r || tmax < -r)
            return {false, INF, {}};
         continue;
      }

      float enter, exit_;
      Vector3 n;
      if (s > 0.0f) {// box approaches from the -L side
         enter = (tmin - r) / s;
         exit_ = (tmax + r) / s;
         n = L * -1.0f;
      } else {// box approaches from the +L side
         enter = (tmax + r) / s;
         exit_ = (tmin - r) / s;
         n = L;
      }

      if (enter > t_enter) {
         t_enter = enter;
         enter_normal = n;
      }
      t_exit = std::min(t_exit, exit_);

      if (t_enter > t_exit || t_exit < 0.0f || t_enter >= 1.0f)
         return {false, INF, {}};
   }

   if (t_enter < 0.0f) {
      // Overlapping at t = 0 on every axis: report an immediate hit with the
      // minimum-translation normal, but only if we're not already leaving.
      if (best_depth == INF)
         return {false, INF, {}};
      if (movement.dot(mtv_normal) >= 0.0f)
         return {false, INF, {}};

      return {true, 0.0f, mtv_normal};
   }

   return {true, t_enter, enter_normal};
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
      const auto sweep_result = sweep_aabb_triangle(traversary.bounding_box, traversary.movement, node->leaf.primitive);
      if (!sweep_result.hit_found) {
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
      return node->bounding_box.intersect_swept(traversary.bounding_box, traversary.movement).entry_time;
   }
};

using MeshBVHSweepTraverser = BVHTraverser<MeshBVHSweepTraversalTraits>;

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

void MeshBVH::clear()
{
   if (m_root == nullptr)
      return;

   MeshBVHUtil::destroy(m_root);
   m_root = nullptr;
}

}// namespace triglav::geometry
