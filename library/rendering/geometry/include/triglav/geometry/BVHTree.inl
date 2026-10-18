#pragma once

#include "BVHUtil.hpp"
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

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload>
[[nodiscard]] BoundingBox node_bounding_box(const TopLevelNode<TIndex, TPayload>* node)
{
   if (node->is_leaf_node) {
      return node->leaf.bounding_box();
   }
   return node->internal.bounding_box;
}

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload>
TopLevelNode<TIndex, TPayload>* build_node(Map<TIndex, TPayload>& leave_mapping, std::span<TPayload> data,
                                           TopLevelNode<TIndex, TPayload>* parent)
{
   if (data.empty())
      return nullptr;
   if (data.size() == 1) {
      auto* node = new TopLevelNode<TIndex, TPayload>{
         .is_leaf_node = true,
         .parent = parent,
         .leaf = std::move(data[0]),
      };
      leave_mapping[node->leaf.index()] = node;
      return node;
   }

   auto bb = calculate_bounding_box<TIndex, TPayload>(data);
   auto axis = bb.major_axis();

   std::sort(data.begin(), data.end(), [axis](const TPayload& left, const TPayload& right) {
      return left.bounding_box().centroid()[axis] > right.bounding_box().centroid()[axis];
   });

   const auto mid = data.size() / 2;

   auto* node = new TopLevelNode<TIndex, TPayload>{
      .is_leaf_node = false,
      .parent = parent,
      .internal =
         {
            .max_surface_area = EXTENSION_MULTIPLIER * bb.half_surface_area(),
            .bounding_box = bb,
         },
   };
   node->internal.left = build_node(leave_mapping, data.subspan(0, mid), node);
   node->internal.right = build_node(leave_mapping, data.subspan(mid, data.size() - mid), node);

   return node;
}

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload>
void update_parent_bounds(TopLevelNode<TIndex, TPayload>* node)
{
   while (node != nullptr) {
      if (node->internal.left != nullptr && node->internal.right != nullptr) {
         node->internal.bounding_box = node_bounding_box(node->internal.left).merge(node_bounding_box(node->internal.right));
      }
      node = node->parent;
   }
}

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload>
struct TopLevelBaseTraits
{
   using Node = TopLevelNode<TIndex, TPayload>;
   using UserData = std::map<TIndex, TopLevelNode<TIndex, TPayload>*>;

   static Node* move_payload(UserData* user_data, Node* old_leaf)
   {
      auto* new_leaf = new Node{
         .is_leaf_node = false,
         .parent = old_leaf,
         .leaf = std::move(old_leaf->leaf),
      };
      user_data->operator[](new_leaf->leaf.index()) = new_leaf;
      return new_leaf;
   }

   static float calculate_insert_cost(UserData* /*user_data*/, const Node* parent, const Node* new_child)
   {
      const auto parent_bb = node_bounding_box(parent);
      const auto child_bb = node_bounding_box(new_child);
      return parent_bb.extension_cost(child_bb);
   }

   static void recalculate_bounds(UserData* /*user_data*/, Node* parent)
   {
      const auto new_bb = node_bounding_box(parent->internal.left).merge(node_bounding_box(parent->internal.right));
      parent->internal.max_surface_area = EXTENSION_MULTIPLIER * new_bb.half_surface_area();
      parent->internal.bounding_box = new_bb;
      parent->internal.left->parent = parent;
      parent->internal.right->parent = parent;
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

   using IntersectionResult = BoundingBox::RayHit;

   static Result test_leaf(const Node* node, const Traversary& traversary, float& best_score)
   {
      assert(node->is_leaf_node);
      auto& payload = node->leaf;

      auto& bottom_level = traversary.bl_provider.get_bottom_level(payload.index());
      const auto bl_ray = traversary.ray.to_local_space(payload.inv_transform_matrix());
      const auto bl_hit = bottom_level.ray_cast(bl_ray);
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
      return inter_result.min_distance;
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
      assert(node != nullptr && node->is_leaf_node);
      auto& payload = node->leaf;

      const SweepResult sweep_result = payload.bounding_box().intersect_swept(traversary.bounding_box, traversary.movement);
      if (sweep_result.status == SweepStatus::Apart || sweep_result.status == SweepStatus::Exiting) {
         return Result::miss();
      }

      auto& bottom_level = traversary.bl_provider.get_bottom_level(payload.index());
      const auto ls_bb = traversary.bounding_box.transform(payload.inv_transform_matrix());
      const auto ls_movement = payload.inv_transform_matrix().shrink() * traversary.movement;
      const auto result = bottom_level.sweep_cast(ls_bb, ls_movement);
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
      .is_leaf_node = false,
      .parent = nullptr,
      .leaf = payload,
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

   auto* sibling = parent->internal.left == leaf ? parent->internal.right : parent->internal.left;
   auto* grandparent = parent->parent;

   sibling->parent = grandparent;
   if (grandparent == nullptr) {
      m_root = sibling;
   } else if (grandparent->internal.left == parent) {
      grandparent->internal.left = sibling;
   } else {
      grandparent->internal.right = sibling;
   }

   parent->internal.left = nullptr;
   parent->internal.right = nullptr;
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
      const auto old_bb = leaf_it->second->leaf.bounding_box();
      const auto new_bb = payload.bounding_box();
      if (old_bb != new_bb) {
         const auto* sibling_node = parent->internal.left == leaf_it->second ? parent->internal.right : parent->internal.left;

         const auto new_parent_bb = new_bb.merge(detail::node_bounding_box(sibling_node));
         needs_to_update_parents = true;

         const auto new_parent_surface_area = new_parent_bb.half_surface_area();
         if (new_parent_surface_area > parent->internal.max_surface_area) {
            this->remove(payload.index());
            this->add(payload);
            return;
         }
      }
   }

   auto* leaf = leaf_it->second;
   leaf->leaf = payload;

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
