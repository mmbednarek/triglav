#pragma once

#include "triglav/Logging.hpp"
#include "triglav/Math.hpp"

namespace triglav::geometry {

namespace detail {

constexpr float EXTENSION_MULTIPLIER = 1.2f;

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload>
using Map = std::map<TIndex, TopLevelNode<TIndex, TPayload>*>;

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload>
[[nodiscard]] BoundingBox calculate_bounding_box(std::span<TPayload> data)
{
   BoundingBox result{
      .min = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()},
      .max = {std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()},
   };
   for (const auto& pd : data) {
      auto bb = pd.bounding_box();
      result.min = {std::min(result.min.x, bb.min.x), std::min(result.min.y, bb.min.y), std::min(result.min.z, bb.min.z)};
      result.max = {std::max(result.max.x, bb.max.x), std::max(result.max.y, bb.max.y), std::max(result.max.z, bb.max.z)};
   }
   return result;
}

inline BoundingBox bounding_box_from_triangle(const std::array<Vector3, 3>& primitive)
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

[[nodiscard]] constexpr BoundingBox merge_bounding_boxes(const BoundingBox& left, const BoundingBox& right)
{
   return {
      .min = {std::min(left.min.x, right.min.x), std::min(left.min.y, right.min.y), std::min(left.min.z, right.min.z)},
      .max = {std::max(left.max.x, right.max.x), std::max(left.max.y, right.max.y), std::max(left.max.z, right.max.z)},
   };
}

template<TriangleMesh TMesh>
[[nodiscard]] BoundingBox calculate_bottom_level_bounding_box(TMesh& mesh, const u32 from, const u32 to)
{
   BoundingBox result{
      .min = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()},
      .max = {std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()},
   };
   for (u32 i = from; i < to; ++i) {
      std::array<Vector3, 3> primitive = mesh.get_primitive(i);
      result = merge_bounding_boxes(bounding_box_from_triangle(primitive), result);
   }
   return result;
}

// NOTE: It's actually half of the surface area
[[nodiscard]] constexpr float bounding_box_surface_area(const BoundingBox& bb)
{
   const auto scale = bb.scale();
   return scale.x * scale.y + scale.x * scale.z + scale.y * scale.z;
}

[[nodiscard]] constexpr float calculate_extension_cost(const BoundingBox& current, const BoundingBox& extension)
{
   return bounding_box_surface_area(merge_bounding_boxes(current, extension)) - bounding_box_surface_area(current);
}

struct TriangleSweepResult
{
   bool hit_found;
   float time;
   Vector3 normal;
};

[[nodiscard]] inline TriangleSweepResult sweep_aabb_triangle(const BoundingBox& bb, const Vector3& movement,
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

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload>
[[nodiscard]] BoundingBox node_bounding_box(const TopLevelNode<TIndex, TPayload>* node)
{
   if (std::holds_alternative<TPayload>(node->payload)) {
      return std::get<TPayload>(node->payload).bounding_box();
   }
   return std::get<BoundingBox>(node->payload);
}

constexpr std::optional<float> ray_triangle_intersect(const std::array<Vector3, 3>& primitive, const Ray& ray)
{
   static constexpr float TOLERANCE = 0.0001f;

   const Vector3 edge1 = primitive[1] - primitive[0];
   const Vector3 edge2 = primitive[2] - primitive[0];

   const Vector3 h = ray.direction.cross(edge2);
   const float a = edge1.dot(h);

   if (a > -TOLERANCE && a < TOLERANCE)
      return std::nullopt;

   const float f = 1.0f / a;
   const Vector3 s = ray.origin - primitive[0];

   const float u = f * s.dot(h);
   if (u < 0.0f || u > 1.0f)
      return std::nullopt;

   const Vector3 q = s.cross(edge1);
   const float v = f * ray.direction.dot(q);
   if (v < 0.0f || (v + u) > 1.0f)
      return std::nullopt;

   const auto t = f * edge2.dot(q);
   if (t < TOLERANCE)
      return std::nullopt;

   return t;
}

inline BottomLevelSweepResult traverse_bottom_level_node_aabb(const BottomLevelNode* node, const BoundingBox& bounding_box,
                                                              const Vector3& movement, float& hit_time)
{
   if (node->primitive_id != ~0u) {
      const auto sweep_result = sweep_aabb_triangle(bounding_box, movement, node->primitive);
      if (!sweep_result.hit_found) {
         return {};
      }

      hit_time = sweep_result.time;

      return {
         .time = hit_time,
         .normal = sweep_result.normal,
         .primitive_id = node->primitive_id,
      };
   }

   const auto bb_left = node->left->bounding_box;
   const bool intersects_left = bb_left.does_intersect_aabb(bounding_box.extend_by_movement(movement));

   const auto bb_right = node->right->bounding_box;
   const bool intersects_right = bb_right.does_intersect_aabb(bounding_box.extend_by_movement(movement));

   if (!intersects_left && !intersects_right) {
      return {};
   }
   if (intersects_left && intersects_right) {
      const auto hit_left = bb_left.intersect_swept(bounding_box, movement);
      const auto hit_right = bb_right.intersect_swept(bounding_box, movement);

      BottomLevelNode* first_node;
      float first_hit{};
      BottomLevelNode* second_node;
      float second_hit{};

      if (hit_left.entry_time < hit_right.entry_time) {
         first_node = node->left;
         first_hit = hit_left.entry_time;
         second_node = node->right;
         second_hit = hit_right.entry_time;
      } else {
         first_node = node->right;
         first_hit = hit_right.entry_time;
         second_node = node->left;
         second_hit = hit_left.entry_time;
      }

      BottomLevelSweepResult result_first{};
      if (first_hit < hit_time) {
         result_first = traverse_bottom_level_node_aabb(first_node, bounding_box, movement, hit_time);
      }
      if (second_hit < hit_time) {
         BottomLevelSweepResult result_second = traverse_bottom_level_node_aabb(second_node, bounding_box, movement, hit_time);
         if (result_second.time < result_first.time) {
            return result_second;
         }
      }
      return result_first;
   }

   BottomLevelNode* child;
   if (intersects_left) {
      child = node->left;
   } else {
      child = node->right;
   }

   return traverse_bottom_level_node_aabb(child, bounding_box, movement, hit_time);
}

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload>
TopLevelNode<TIndex, TPayload>* build_node(Map<TIndex, TPayload>& leave_mapping, std::span<TPayload> data,
                                           TopLevelNode<TIndex, TPayload>* parent)
{
   if (data.empty())
      return nullptr;
   if (data.size() == 1) {
      auto* node = new TopLevelNode<TIndex, TPayload>{
         .payload = std::move(data[0]),
         .parent = parent,
         .left = nullptr,
         .right = nullptr,
      };
      leave_mapping[std::get<TPayload>(node->payload).index()] = node;
      return node;
   }

   auto bb = calculate_bounding_box<TIndex, TPayload>(data);
   auto axis = bb.major_axis();

   std::sort(data.begin(), data.end(), [axis](const TPayload& left, const TPayload& right) {
      return left.bounding_box().centroid()[axis] > right.bounding_box().centroid()[axis];
   });

   const auto mid = data.size() / 2;

   auto* node = new TopLevelNode<TIndex, TPayload>{
      .payload = bb,
      .max_surface_area = EXTENSION_MULTIPLIER * bounding_box_surface_area(bb),
      .parent = parent,
   };
   node->left = build_node(leave_mapping, data.subspan(0, mid), node);
   node->right = build_node(leave_mapping, data.subspan(mid, data.size() - mid), node);

   return node;
}

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload>
void update_parent_bounds(TopLevelNode<TIndex, TPayload>* node)
{
   while (node != nullptr) {
      if (node->left != nullptr && node->right != nullptr) {
         node->payload = merge_bounding_boxes(node_bounding_box(node->left), node_bounding_box(node->right));
      }
      node = node->parent;
   }
}

struct BottomLevelBaseTraits
{
   using Node = BottomLevelNode;
   using UserData = std::monostate;

   static Node* move_payload(const UserData* /*user_data*/, Node* leaf_node)
   {
      return new BottomLevelNode{
         .primitive_id = std::exchange(leaf_node->primitive_id, ~0u),
         .bounding_box = leaf_node->bounding_box,
         .left = nullptr,
         .right = nullptr,
         .primitive = std::exchange(leaf_node->primitive, {}),
      };
   }

   static float calculate_insert_cost(UserData* /*user_data*/, const Node* parent, const Node* new_child)
   {
      return calculate_extension_cost(parent->bounding_box, new_child->bounding_box);
   }

   static void recalculate_bounds(const UserData* /*user_data*/, Node* parent)
   {
      parent->bounding_box = merge_bounding_boxes(parent->left->bounding_box, parent->right->bounding_box);
   }
};

using BottomLevelUtils = BVHUtil<BottomLevelBaseTraits>;

struct BottomLevelRayTraversalTraits
{
   using BaseTraits = BottomLevelBaseTraits;
   using Node = BaseTraits::Node;
   using Result = BottomLevelHit;
   using Traversary = Ray;

   struct IntersectionResult
   {
      std::optional<Vector2> result;

      [[nodiscard]] bool has_hit() const
      {
         return result.has_value();
      }
   };

   static Result test_leaf(const Node* node, const Ray& ray, float& best_score)
   {
      const auto distance = ray_triangle_intersect(node->primitive, ray);

      if (!distance.has_value() || *distance > best_score)
         return Result::miss();

      best_score = *distance;
      return {*distance, node->primitive_id};
   }

   static IntersectionResult test_intersection(const Node* node, const Ray& ray)
   {
      return IntersectionResult{node->bounding_box.intersect(ray)};
   }

   static float test_hit_score(const IntersectionResult& inter_result, const Node* /*node*/, const Ray& /*traversary*/)
   {
      assert(inter_result.has_hit());
      return inter_result.result->x;
   }
};

using BottomLevelRayTraverser = BVHTraverser<BottomLevelRayTraversalTraits>;

struct BottomLevelSweepTraversalTraits
{
   using BaseTraits = BottomLevelBaseTraits;
   using Node = BaseTraits::Node;
   using Result = BottomLevelSweepResult;
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

   static Result test_leaf(const Node* node, const Traversary& traversary, float& best_score)
   {
      const auto sweep_result = sweep_aabb_triangle(traversary.bounding_box, traversary.movement, node->primitive);
      if (!sweep_result.hit_found) {
         return {};
      }

      best_score = sweep_result.time;

      return {
         .time = sweep_result.time,
         .normal = sweep_result.normal,
         .primitive_id = node->primitive_id,
      };
   }

   static IntersectionResult test_intersection(const Node* node, const Traversary& traversary)
   {
      return IntersectionResult{node->bounding_box.does_intersect_aabb(traversary.bounding_box.extend_by_movement(traversary.movement))};
   }

   static float test_hit_score(const IntersectionResult& /*inter_result*/, const Node* node, const Traversary& traversary)
   {
      return node->bounding_box.intersect_swept(traversary.bounding_box, traversary.movement).entry_time;
   }
};

using BottomLevelSweepTraverser = BVHTraverser<BottomLevelSweepTraversalTraits>;

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload>
struct TopLevelBaseTraits
{
   using Node = TopLevelNode<TIndex, TPayload>;
   using UserData = std::map<TIndex, TopLevelNode<TIndex, TPayload>*>;

   static Node* move_payload(UserData* user_data, Node* old_leaf)
   {
      auto* new_leaf = new Node{
         .payload = std::move(std::get<TPayload>(old_leaf->payload)),
         .parent = old_leaf,
         .left = nullptr,
         .right = nullptr,
      };
      user_data->operator[](std::get<TPayload>(new_leaf->payload).index()) = new_leaf;
      return new_leaf;
   }

   static float calculate_insert_cost(UserData* /*user_data*/, const Node* parent, const Node* new_child)
   {
      const auto parent_bb = node_bounding_box(parent);
      const auto child_bb = node_bounding_box(new_child);
      return calculate_extension_cost(parent_bb, child_bb);
   }

   static void recalculate_bounds(UserData* /*user_data*/, Node* parent)
   {
      const auto new_bb = merge_bounding_boxes(node_bounding_box(parent->left), node_bounding_box(parent->right));
      parent->max_surface_area = EXTENSION_MULTIPLIER * bounding_box_surface_area(new_bb);
      parent->payload = new_bb;
      parent->left->parent = parent;
      parent->right->parent = parent;
   }
};

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload>
using TopLevelUtil = BVHUtil<TopLevelBaseTraits<TIndex, TPayload>>;

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload, typename TBottomLevel,
         BottomLevelProvider<TIndex, TBottomLevel> TBottomLevelProvider>
struct TopLevelRayTraversalTraits
{
   using BaseTraits = TopLevelBaseTraits<TIndex, TPayload>;
   using Node = BaseTraits::Node;
   using Result = TopLevelHit<TIndex, TPayload>;
   struct Traversary
   {
      const Ray& ray;
      const TBottomLevelProvider& bl_provider;
   };

   struct IntersectionResult
   {
      std::optional<Vector2> result;

      [[nodiscard]] bool has_hit() const
      {
         return result.has_value();
      }
   };

   static Result test_leaf(const Node* node, const Traversary& traversary, float& best_score)
   {
      auto& payload = std::get<TPayload>(node->payload);

      auto& bottom_level = traversary.bl_provider.get_bottom_level(payload.index());
      const auto bl_ray = traversary.ray.to_local_space(payload.inv_transform_matrix());
      const auto bl_hit = bottom_level.traverse(bl_ray);
      if (bl_hit.distance > best_score)
         return Result::miss();

      best_score = bl_hit.distance;
      return Result{bl_hit.distance, &payload};
   }

   static IntersectionResult test_intersection(const Node* node, const Traversary& traversary)
   {
      return IntersectionResult{node_bounding_box(node).intersect(traversary.ray)};
   }

   static float test_hit_score(const IntersectionResult& inter_result, const Node* /*node*/, const Traversary& /*traversary*/)
   {
      assert(inter_result.has_hit());
      return inter_result.result->x;
   }
};

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload, typename TBottomLevel,
         BottomLevelProvider<TIndex, TBottomLevel> TBottomLevelProvider>
using TopLevelRayTraverser = BVHTraverser<TopLevelRayTraversalTraits<TIndex, TPayload, TBottomLevel, TBottomLevelProvider>>;

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload, typename TBottomLevel,
         BottomLevelProvider<TIndex, TBottomLevel> TBottomLevelProvider>
struct TopLevelSweepTraversalTraits
{
   using BaseTraits = TopLevelBaseTraits<TIndex, TPayload>;
   using Node = BaseTraits::Node;
   using Result = TopLevelSweptHit<TIndex, TPayload>;
   struct Traversary
   {
      const BoundingBox& bounding_box;
      Vector3 movement;
      const TBottomLevelProvider& bl_provider;
   };

   struct IntersectionResult
   {
      bool does_intersect;

      [[nodiscard]] bool has_hit() const
      {
         return does_intersect;
      }
   };

   static Result test_leaf(const Node* node, const Traversary& traversary, float& best_score)
   {
      auto& payload = std::get<TPayload>(node->payload);

      const SweepResult sweep_result = payload.bounding_box().intersect_swept(traversary.bounding_box, traversary.movement);
      if (sweep_result.status == SweepStatus::Apart || sweep_result.status == SweepStatus::Exiting) {
         return Result::miss();
      }

      auto& bottom_level = traversary.bl_provider.get_bottom_level(payload.index());
      const auto ls_bb = traversary.bounding_box.transform(payload.inv_transform_matrix());
      const auto ls_movement = payload.inv_transform_matrix().shrink() * traversary.movement;
      const BottomLevelSweepResult result = bottom_level.traverse_swept(ls_bb, ls_movement);
      if (result.primitive_id == ~0u) {
         return Result::miss();
      }
      if (result.time >= best_score) {
         return Result::miss();
      }
      best_score = result.time;

      return Result{
         .time = result.time,
         .normal = (payload.inv_transform_matrix().shrink().transpose() * result.normal).normalize(),
         .status = sweep_result.status,
         .payload = &payload,
      };
   }

   static IntersectionResult test_intersection(const Node* node, const Traversary& traversary)
   {
      return IntersectionResult{
         node_bounding_box(node).does_intersect_aabb(traversary.bounding_box.extend_by_movement(traversary.movement))};
   }

   static float test_hit_score(const IntersectionResult& /*inter_result*/, const Node* node, const Traversary& traversary)
   {
      return node_bounding_box(node).intersect_swept(traversary.bounding_box, traversary.movement).entry_time;
   }
};

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload, typename TBottomLevel,
         BottomLevelProvider<TIndex, TBottomLevel> TBottomLevelProvider>
using TopLevelSweepTraverser = BVHTraverser<TopLevelSweepTraversalTraits<TIndex, TPayload, TBottomLevel, TBottomLevelProvider>>;

}// namespace detail

// BOTTOM LEVEL BVH

template<TriangleMesh TMesh>
BottomLevelBVH<TMesh>::~BottomLevelBVH()
{
   this->clear();
}

template<TriangleMesh TMesh>
BottomLevelBVH<TMesh>::BottomLevelBVH(BottomLevelBVH&& other) noexcept :
    m_root(std::exchange(other.m_root, nullptr))
{
}

template<TriangleMesh TMesh>
BottomLevelBVH<TMesh>& BottomLevelBVH<TMesh>::operator=(BottomLevelBVH&& other) noexcept
{
   if (this == &other)
      return *this;
   m_root = std::exchange(other.m_root, nullptr);
   return *this;
}

template<TriangleMesh TMesh>
void BottomLevelBVH<TMesh>::build(TMesh& mesh)
{
   this->clear();

   const auto primitive_count = mesh.primitive_count();
   if (primitive_count == 0)
      return;

   auto base_primitive = mesh.get_primitive(0);
   m_root = new BottomLevelNode{
      .primitive_id = 0,
      .bounding_box = detail::bounding_box_from_triangle(base_primitive),
      .left = nullptr,
      .right = nullptr,
      .primitive = base_primitive,
   };
   for (u32 i = 1; i < primitive_count; ++i) {
      auto primitive = mesh.get_primitive(i);
      detail::BottomLevelUtils::insert_node(nullptr, m_root,
                                            new BottomLevelNode{
                                               .primitive_id = i,
                                               .bounding_box = detail::bounding_box_from_triangle(primitive),
                                               .left = nullptr,
                                               .right = nullptr,
                                               .primitive = primitive,
                                            });
   }
}

template<TriangleMesh TMesh>
BottomLevelHit BottomLevelBVH<TMesh>::traverse(const Ray& ray) const
{
   if (m_root == nullptr)
      return {INFINITY, ~0u};

   const auto intersect = m_root->bounding_box.intersect(ray);
   if (!intersect.has_value())
      return {INFINITY, ~0u};

   Ray mut_ray = ray;
   return detail::BottomLevelRayTraverser::traverse(m_root, mut_ray, mut_ray.distance);
}

template<TriangleMesh TMesh>
BottomLevelSweepResult BottomLevelBVH<TMesh>::traverse_swept(const BoundingBox& bb, const Vector3& movement) const
{
   float hit_time = INF;
   return detail::BottomLevelSweepTraverser::traverse(m_root, {bb, movement}, hit_time);
}

template<TriangleMesh TMesh>
void BottomLevelBVH<TMesh>::clear()
{
   if (m_root != nullptr) {
      detail::BottomLevelUtils::destroy(m_root);
   }
   m_root = nullptr;
}

// TOP LEVEL BVH

#define TOP_LEVEL(...)                                                                  \
   template<typename TIndex, TopLevelPrimitive<TIndex> TPayload, typename TBottomLevel, \
            BottomLevelProvider<TIndex, TBottomLevel> TBottomLevelProvider>             \
   __VA_ARGS__ TopLevelBVH<TIndex, TPayload, TBottomLevel, TBottomLevelProvider>

TOP_LEVEL()::TopLevelBVH(TBottomLevelProvider& provider) :
    m_bl_provider(provider)
{
}

TOP_LEVEL()::TopLevelBVH(TopLevelBVH&& other) noexcept :
    m_root(std::exchange(other.m_root, nullptr)),
    m_bl_provider(other.m_bl_provider)
{
}

TOP_LEVEL()::~TopLevelBVH()
{
   this->clear();
}

TOP_LEVEL(TopLevelBVH<TIndex, TPayload, TBottomLevel, TBottomLevelProvider>&)::operator=(TopLevelBVH && other) noexcept
{
   if (this == &other)
      return *this;
   m_root = std::exchange(other.m_root, nullptr);

   return *this;
}

TOP_LEVEL(void)::add(const TPayload& payload)
{
   auto* node = new TopLevelNode<TIndex, TPayload>{
      .payload = payload,
      .left = nullptr,
      .right = nullptr,
   };

   m_leave_mapping[payload.index()] = node;

   if (m_root == nullptr) {
      m_root = node;
   } else {
      detail::TopLevelUtil<TIndex, TPayload>::insert_node(&this->m_leave_mapping, m_root, node);
   }

   assert(node->is_leaf());
}

TOP_LEVEL(void)::remove(TIndex index)
{
   const auto leaf_it = m_leave_mapping.find(index);
   if (leaf_it == m_leave_mapping.end()) {
      return;
   }

   auto* leaf = leaf_it->second;
   m_leave_mapping.erase(leaf_it);

   auto* parent = leaf->parent;
   if (parent == nullptr) {
      delete leaf;
      m_root = nullptr;
      return;
   }

   auto* sibling = parent->left == leaf ? parent->right : parent->left;
   auto* grandparent = parent->parent;

   sibling->parent = grandparent;
   if (grandparent == nullptr) {
      m_root = sibling;
   } else if (grandparent->left == parent) {
      grandparent->left = sibling;
   } else {
      grandparent->right = sibling;
   }

   parent->left = nullptr;
   parent->right = nullptr;
   delete leaf;
   delete parent;

   detail::update_parent_bounds(grandparent);
}

TOP_LEVEL(void)::update(const TPayload& payload)
{
   const auto leaf_it = m_leave_mapping.find(payload.index());
   if (leaf_it == m_leave_mapping.end()) {
      return;
   }

   bool needs_to_update_parents = false;

   auto* parent = leaf_it->second->parent;
   if (parent != nullptr) {
      assert(leaf_it->second->is_leaf());
      const auto old_bb = std::get<TPayload>(leaf_it->second->payload).bounding_box();
      const auto new_bb = payload.bounding_box();
      if (old_bb != new_bb) {
         const auto* sibling_node = parent->left == leaf_it->second ? parent->right : parent->left;

         const auto new_parent_bb = detail::merge_bounding_boxes(new_bb, detail::node_bounding_box(sibling_node));
         needs_to_update_parents = true;

         const auto new_parent_surface_area = detail::bounding_box_surface_area(new_parent_bb);
         if (new_parent_surface_area > parent->max_surface_area) {
            this->remove(payload.index());
            this->add(payload);
            return;
         }
      }
   }

   auto* leaf = leaf_it->second;
   leaf->payload = payload;

   if (needs_to_update_parents) {
      detail::update_parent_bounds(leaf->parent);
   }
}

TOP_LEVEL(void)::build(std::span<TPayload> data)
{
   this->clear();
   m_root = detail::build_node<TIndex, TPayload>(m_leave_mapping, data, nullptr);
}

TOP_LEVEL(void)::clear()
{
   if (m_root != nullptr) {
      detail::TopLevelUtil<TIndex, TPayload>::destroy(m_root);
   }
}

TOP_LEVEL(TopLevelHit<TIndex, TPayload>)::traverse(const Ray& ray) const
{
   if (m_root == nullptr) {
      return {INFINITY, nullptr};
   }
   Ray mut_ray = ray;
   return detail::TopLevelRayTraverser<TIndex, TPayload, TBottomLevel, TBottomLevelProvider>::traverse(m_root, {mut_ray, m_bl_provider},
                                                                                                       mut_ray.distance);
}

TOP_LEVEL(TopLevelSweptHit<TIndex, TPayload>)::traverse_swept(const BoundingBox& bb, const Vector3& movement) const
{
   float hit_time = INF;
   return detail::TopLevelSweepTraverser<TIndex, TPayload, TBottomLevel, TBottomLevelProvider>::traverse(
      m_root, {bb, movement, m_bl_provider}, hit_time);
}

TOP_LEVEL(const TPayload&)::get(TIndex index)
{
   return std::get<TPayload>(m_leave_mapping.at(index)->payload);
}

#undef TOP_LEVEL

}// namespace triglav::geometry
