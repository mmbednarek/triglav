#pragma once

#include "triglav/Math.hpp"

namespace triglav::geometry {

namespace detail {

constexpr float EXTENSION_MULTIPLIER = 1.2f;

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload>
using Map = std::map<TIndex, TopLevelNode<TIndex, TPayload>*>;

constexpr Axis get_major_axis(const BoundingBox& bb)
{
   const Vector3 extend = bb.scale();

   Axis result = Axis::X;
   auto max = extend.x;

   if (extend.y > max) {
      result = Axis::Y;
      max = extend.y;
   }
   if (extend.z > max) {
      result = Axis::Z;
   }

   return result;
}

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload>
void delete_node(const TopLevelNode<TIndex, TPayload>* node)
{
   if (node->left != nullptr) {
      delete_node(node->left);
   }
   if (node->right != nullptr) {
      delete_node(node->right);
   }
   delete node;
}

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

inline void delete_bottom_level_node(const BottomLevelNode* node)
{
   if (node->left != nullptr) {
      delete_bottom_level_node(node->left);
   }
   if (node->right != nullptr) {
      delete_bottom_level_node(node->right);
   }
   delete node;
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

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload>
[[nodiscard]] BoundingBox node_bounding_box(const TopLevelNode<TIndex, TPayload>* node)
{
   if (std::holds_alternative<TPayload>(node->payload)) {
      return std::get<TPayload>(node->payload).bounding_box();
   }
   return std::get<BoundingBox>(node->payload);
}

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload>
TopLevelNode<TIndex, TPayload>* insert_node(TopLevelNode<TIndex, TPayload>* node, const TPayload& payload)
{
   if (std::holds_alternative<TPayload>(node->payload)) {
      auto* old_leaf = new TopLevelNode<TIndex, TPayload>{
         .payload = std::move(std::get<TPayload>(node->payload)),
         .parent = node,
         .left = nullptr,
         .right = nullptr,
      };
      auto* new_leaf = new TopLevelNode<TIndex, TPayload>{
         .payload = payload,
         .parent = node,
         .left = nullptr,
         .right = nullptr,
      };

      const auto new_bb = merge_bounding_boxes(std::get<TPayload>(old_leaf->payload).bounding_box(), payload.bounding_box());
      node->payload = new_bb;
      node->max_surface_area = EXTENSION_MULTIPLIER * bounding_box_surface_area(new_bb);
      node->left = old_leaf;
      node->right = new_leaf;
      return new_leaf;
   }

   const auto payload_bb = payload.bounding_box();
   const auto left_bb = node_bounding_box(node->left);
   const auto right_bb = node_bounding_box(node->right);

   const auto left_cost = calculate_extension_cost(left_bb, payload_bb);
   const auto right_cost = calculate_extension_cost(right_bb, payload_bb);

   TopLevelNode<TIndex, TPayload>* result{};
   if (left_cost < right_cost) {
      result = insert_node(node->left, payload);
   } else {
      result = insert_node(node->right, payload);
   }

   std::get<BoundingBox>(node->payload) = merge_bounding_boxes(node_bounding_box(node->left), node_bounding_box(node->right));
   return result;
}

template<TriangleMesh TMesh>
BottomLevelNode* insert_bottom_level_node(BottomLevelNode* node, TMesh& mesh, const u32 primitive_index)
{
   auto primitive = mesh.get_primitive(primitive_index);
   auto primitive_bb = bounding_box_from_triangle(primitive);
   if (node->primitive_id < mesh.primitive_count()) {
      auto* old_leaf = new BottomLevelNode{
         .primitive_id = node->primitive_id,
         .bounding_box = node->bounding_box,
         .left = nullptr,
         .right = nullptr,
         .primitive = node->primitive,
      };
      auto* new_leaf = new BottomLevelNode{
         .primitive_id = primitive_index,
         .bounding_box = primitive_bb,
         .left = nullptr,
         .right = nullptr,
         .primitive = primitive,
      };

      node->primitive_id = ~0u;
      node->bounding_box = merge_bounding_boxes(old_leaf->bounding_box, new_leaf->bounding_box);
      node->left = old_leaf;
      node->right = new_leaf;
      node->primitive = {};
      return new_leaf;
   }

   const auto left_cost = calculate_extension_cost(node->left->bounding_box, primitive_bb);
   const auto right_cost = calculate_extension_cost(node->right->bounding_box, primitive_bb);

   BottomLevelNode* result{};
   if (left_cost < right_cost) {
      result = insert_bottom_level_node(node->left, mesh, primitive_index);
   } else {
      result = insert_bottom_level_node(node->right, mesh, primitive_index);
   }

   node->bounding_box = merge_bounding_boxes(node->left->bounding_box, node->right->bounding_box);

   return result;
}

constexpr std::optional<float> ray_triangle_intersect(const std::array<Vector3, 3>& primitive, const Ray& ray)
{
   static constexpr float TOLERANCE = 0.0001f;

   const Vector3 edge1 = primitive[1] - primitive[0];
   const Vector3 edge2 = primitive[2] - primitive[0];

   const Vector3 h = glm::cross(ray.direction, edge2);
   const float a = glm::dot(edge1, h);

   if (a > -TOLERANCE && a < TOLERANCE)
      return std::nullopt;

   const float f = 1.0f / a;
   const Vector3 s = ray.origin - primitive[0];

   const float u = f * glm::dot(s, h);
   if (u < 0.0f || u > 1.0f)
      return std::nullopt;

   const Vector3 q = glm::cross(s, edge1);
   const float v = f * glm::dot(ray.direction, q);
   if (v < 0.0f || (v + u) > 1.0f)
      return std::nullopt;

   const auto t = f * glm::dot(edge2, q);
   if (t < TOLERANCE)
      return std::nullopt;

   return t;
}


inline BottomLevelHit traverse_bottom_level_node(const BottomLevelNode* node, Ray& ray)
{
   if (node->primitive_id != ~0u) {
      const auto primitive = node->primitive;
      const auto distance = ray_triangle_intersect(primitive, ray);

      if (!distance.has_value() || *distance > ray.distance)
         return {INFINITY, ~0u};

      ray.distance = *distance;
      return {*distance, node->primitive_id};
   }

   const auto left_intersect = node->left->bounding_box.intersect(ray);
   const auto right_intersect = node->right->bounding_box.intersect(ray);

   if (!left_intersect.has_value() && !right_intersect.has_value())
      return {INFINITY, ~0u};

   if (left_intersect.has_value() && right_intersect.has_value()) {
      BottomLevelNode* closest{};
      BottomLevelNode* furthest{};

      float furthest_distance{};

      if (left_intersect->x < right_intersect->x) {
         closest = node->left;
         furthest = node->right;
         furthest_distance = right_intersect->x;
      } else {
         closest = node->right;
         furthest = node->left;
         furthest_distance = left_intersect->x;
      }

      const auto closest_hit = traverse_bottom_level_node(closest, ray);

      if (ray.distance > furthest_distance) {
         const auto furthest_hit = traverse_bottom_level_node(furthest, ray);
         if (furthest_hit.distance <= ray.distance) {
            return furthest_hit;
         }
      }
      return closest_hit;
   }

   BottomLevelNode* child{};
   if (left_intersect.has_value()) {
      child = node->left;
   } else {
      child = node->right;
   }

   return traverse_bottom_level_node(child, ray);
}

template<typename TCallback>
bool traverse_bottom_level_node_aabb(BottomLevelNode* node, const BoundingBox& bounding_box, TCallback callback)
{
   if (node->primitive_id != ~0u) {
      return callback(node->bounding_box, node->primitive_id);
   }

   if (node->left->bounding_box.does_intersect_aabb(bounding_box)) {
      if (traverse_bottom_level_node_aabb(node->left, bounding_box, callback))
         return true;
   }
   if (node->right->bounding_box.does_intersect_aabb(bounding_box)) {
      if (traverse_bottom_level_node_aabb(node->right, bounding_box, callback))
         return true;
   }

   return false;
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

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload, typename TBottomLevel,
         BottomLevelProvider<TIndex, TBottomLevel> TBottomLevelProvider>
TopLevelHit<TIndex, TPayload> traverse_node(const TBottomLevelProvider& bl_provider, TopLevelNode<TIndex, TPayload>* node, Ray& ray)
{
   if (std::holds_alternative<TPayload>(node->payload)) {
      auto& payload = std::get<TPayload>(node->payload);

      auto& bottom_level = bl_provider.get_bottom_level(payload.index());
      const auto bl_ray = ray.to_local_space(payload.inv_transform_matrix());
      const auto bl_hit = bottom_level.traverse(bl_ray);
      if (bl_hit.distance > ray.distance)
         return {INFINITY, nullptr};

      ray.distance = bl_hit.distance;
      return {bl_hit.distance, &payload};
   }

   const auto left_intersect = node_bounding_box(node->left).intersect(ray);
   const auto right_intersect = node_bounding_box(node->right).intersect(ray);

   if (!left_intersect.has_value() && !right_intersect.has_value())
      return {INFINITY, nullptr};

   if (left_intersect.has_value() && right_intersect.has_value()) {
      TopLevelNode<TIndex, TPayload>* closest{};
      TopLevelNode<TIndex, TPayload>* furthest{};

      float furthest_distance{};

      if (left_intersect->x < right_intersect->x) {
         closest = node->left;
         furthest = node->right;
         furthest_distance = right_intersect->x;
      } else {
         closest = node->right;
         furthest = node->left;
         furthest_distance = left_intersect->x;
      }

      const auto closest_hit = traverse_node<TIndex, TPayload, TBottomLevel, TBottomLevelProvider>(bl_provider, closest, ray);

      if (ray.distance > furthest_distance) {
         const auto furthest_hit = traverse_node<TIndex, TPayload, TBottomLevel, TBottomLevelProvider>(bl_provider, furthest, ray);
         if (furthest_hit.distance <= ray.distance)
            return furthest_hit;
      }
      return closest_hit;
   }

   TopLevelNode<TIndex, TPayload>* child{};
   if (left_intersect.has_value()) {
      child = node->left;
   } else {
      child = node->right;
   }

   return traverse_node<TIndex, TPayload, TBottomLevel, TBottomLevelProvider>(bl_provider, child, ray);
}

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload, typename TCallback, typename TBottomLevel,
         BottomLevelProvider<TIndex, TBottomLevel> TBottomLevelProvider>
void traverse_node_aabb(TBottomLevelProvider& provider, TopLevelNode<TIndex, TPayload>* node, const BoundingBox& bounding_box,
                        TCallback callback)
{
   if (std::holds_alternative<TPayload>(node->payload)) {
      auto& payload = std::get<TPayload>(node->payload);
      // auto& bottom_level = provider.get_bottom_level(payload.index());
      // bool intersects = false;
      // auto local_space_bb = bounding_box.transform(payload.inv_transform_matrix());
      // bottom_level.traverse_aabb(local_space_bb, [&]([[maybe_unused]] const BoundingBox& bounding_box, u32 /*primitive_id*/) {
      //    intersects = true;
      //    return true;
      // });
      // if (intersects) {
      callback(payload);
      // }
      return;
   }

   if (node_bounding_box(node->left).does_intersect_aabb(bounding_box)) {
      traverse_node_aabb<TIndex, TPayload, TCallback, TBottomLevel, TBottomLevelProvider>(provider, node->left, bounding_box, callback);
   }
   if (node_bounding_box(node->right).does_intersect_aabb(bounding_box)) {
      traverse_node_aabb<TIndex, TPayload, TCallback, TBottomLevel, TBottomLevelProvider>(provider, node->right, bounding_box, callback);
   }
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

   m_root = new BottomLevelNode{
      .primitive_id = 0,
      .bounding_box = detail::bounding_box_from_triangle(mesh.get_primitive(0)),
      .left = nullptr,
      .right = nullptr,
   };
   for (u32 i = 1; i < primitive_count; ++i) {
      detail::insert_bottom_level_node(m_root, mesh, i);
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
   return detail::traverse_bottom_level_node(m_root, mut_ray);
}

template<TriangleMesh TMesh>
template<typename TCallback>
void BottomLevelBVH<TMesh>::traverse_aabb(const BoundingBox& bb, TCallback callback) const
{
   detail::traverse_bottom_level_node_aabb(m_root, bb, callback);
}

template<TriangleMesh TMesh>
void BottomLevelBVH<TMesh>::clear()
{
   if (m_root != nullptr) {
      detail::delete_bottom_level_node(m_root);
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
   auto* node = detail::insert_node(m_root, payload);
   m_leave_mapping[payload.index()] = node;
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
      const auto current_bb = std::get<BoundingBox>(parent->payload);
      const auto new_bb = detail::merge_bounding_boxes(current_bb, payload.bounding_box());
      if (new_bb != current_bb) {
         needs_to_update_parents = true;

         const auto new_parent_surface_area = detail::bounding_box_surface_area(new_bb);
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
      detail::delete_node(m_root);
   }
}

TOP_LEVEL(TopLevelHit<TIndex, TPayload>)::traverse(const Ray& ray) const
{
   if (m_root == nullptr) {
      return {INFINITY, nullptr};
   }
   Ray mut_ray = ray;
   return detail::traverse_node<TIndex, TPayload, TBottomLevel, TBottomLevelProvider>(m_bl_provider, m_root, mut_ray);
}

TOP_LEVEL(template<typename TCallback> void)::traverse_aabb(const BoundingBox& bb, TCallback callback) const
{
   detail::traverse_node_aabb<TIndex, TPayload, TCallback, TBottomLevel, TBottomLevelProvider>(m_bl_provider, m_root, bb, callback);
}

TOP_LEVEL(const TPayload&)::get(TIndex index)
{
   return std::get<TPayload>(m_leave_mapping.at(index)->payload);
}

#undef TOP_LEVEL

}// namespace triglav::geometry
