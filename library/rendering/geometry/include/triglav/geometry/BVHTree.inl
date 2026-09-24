#pragma once

#include "triglav/Math.hpp"

#include <numeric>

namespace triglav::geometry {

namespace detail {

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
using Map = std::map<TIndex, BVHNode<TIndex, TPayload>*>;

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

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
void delete_node(const BVHNode<TIndex, TPayload>* node)
{
   if (node->left != nullptr) {
      delete_node(node->left);
   }
   if (node->right != nullptr) {
      delete_node(node->right);
   }
   delete node;
}

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
[[nodiscard]] BoundingBox calculate_bounding_box(std::span<TPayload> data)
{
   BoundingBox result{
      .min = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()},
      .max = {std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()},
   };
   for (const auto pd : data) {
      auto bb = pd.bounding_box();
      result.min = {std::min(result.min.x, bb.min.x), std::min(result.min.y, bb.min.y), std::min(result.min.z, bb.min.z)};
      result.max = {std::max(result.max.x, bb.max.x), std::max(result.max.y, bb.max.y), std::max(result.max.z, bb.max.z)};
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

[[nodiscard]] constexpr float bounding_box_volume(const BoundingBox& bb)
{
   const auto scale = bb.scale();
   return scale.x * scale.y * scale.z;
}

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
[[nodiscard]] BoundingBox node_bounding_box(const BVHNode<TIndex, TPayload>* node)
{
   if (std::holds_alternative<TPayload>(node->payload)) {
      return std::get<TPayload>(node->payload).bounding_box();
   }
   return std::get<BoundingBox>(node->payload);
}

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
BVHNode<TIndex, TPayload>* insert_node(BVHNode<TIndex, TPayload>* node, const TPayload& payload)
{
   if (std::holds_alternative<TPayload>(node->payload)) {
      auto* old_leaf = new BVHNode<TIndex, TPayload>{
         .payload = std::move(std::get<TPayload>(node->payload)),
         .parent = node,
         .left = nullptr,
         .right = nullptr,
      };
      auto* new_leaf = new BVHNode<TIndex, TPayload>{
         .payload = payload,
         .parent = node,
         .left = nullptr,
         .right = nullptr,
      };

      node->payload = merge_bounding_boxes(std::get<TPayload>(old_leaf->payload).bounding_box(), payload.bounding_box());
      node->left = old_leaf;
      node->right = new_leaf;
      return new_leaf;
   }

   const auto payload_bb = payload.bounding_box();
   const auto left_bb = node_bounding_box(node->left);
   const auto right_bb = node_bounding_box(node->right);

   const auto left_cost = bounding_box_volume(merge_bounding_boxes(left_bb, payload_bb)) - bounding_box_volume(left_bb);
   const auto right_cost = bounding_box_volume(merge_bounding_boxes(right_bb, payload_bb)) - bounding_box_volume(right_bb);

   BVHNode<TIndex, TPayload>* result{};
   if (left_cost < right_cost) {
      result = insert_node(node->left, payload);
   } else {
      result = insert_node(node->right, payload);
   }

   std::get<BoundingBox>(node->payload) = merge_bounding_boxes(node_bounding_box(node->left), node_bounding_box(node->right));
   return result;
}

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
BVHNode<TIndex, TPayload>* build_node(Map<TIndex, TPayload>& leave_mapping, std::span<TPayload> data, BVHNode<TIndex, TPayload>* parent)
{
   if (data.empty())
      return nullptr;
   if (data.size() == 1) {
      auto* node = new BVHNode<TIndex, TPayload>{
         .payload = std::move(data[0]),
         .parent = parent,
         .left = nullptr,
         .right = nullptr,
      };
      leave_mapping[std::get<TPayload>(node->payload).index()] = node;
      return node;
   }

   auto bb = calculate_bounding_box<TIndex, TPayload>(data);
   auto axis = get_major_axis(bb);

   std::sort(data.begin(), data.end(), [axis](const TPayload& left, const TPayload& right) {
      return vector3_component(left.bounding_box().centroid(), axis) > vector3_component(right.bounding_box().centroid(), axis);
   });

   const auto mid = data.size() / 2;

   auto* node = new BVHNode<TIndex, TPayload>{
      .payload = bb,
      .parent = parent,
   };
   node->left = build_node(leave_mapping, data.subspan(0, mid), node);
   node->right = build_node(leave_mapping, data.subspan(mid, data.size() - mid), node);

   return node;
}

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
BVHHit<TIndex, TPayload> traverse_node(BVHNode<TIndex, TPayload>* node, const Ray& ray)
{
   if (std::holds_alternative<TPayload>(node->payload)) {
      auto& payload = std::get<TPayload>(node->payload);
      auto bb = payload.bounding_box();
      const auto bb_intersect = bb.intersect(ray);
      if (!bb_intersect.has_value()) {
         return {INFINITY, nullptr};
      }
      return {bb_intersect->x, &std::get<TPayload>(node->payload)};
   }

   const auto& bb = std::get<BoundingBox>(node->payload);
   if (!bb.does_intersect(ray)) {
      return {INFINITY, nullptr};
   }

   const auto left_node = traverse_node(node->left, ray);
   const auto right_node = traverse_node(node->right, ray);
   if (right_node.distance < 0 && left_node.payload != nullptr) {
      return left_node;
   }
   if (left_node.distance < 0 && right_node.payload != nullptr) {
      return right_node;
   }

   if (left_node.distance < right_node.distance) {
      return left_node;
   }
   return right_node;
}

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
void update_parent_bounds(BVHNode<TIndex, TPayload>* node)
{
   while (node != nullptr) {
      if (node->left != nullptr && node->right != nullptr) {
         node->payload = merge_bounding_boxes(node_bounding_box(node->left), node_bounding_box(node->right));
      }
      node = node->parent;
   }
}

}// namespace detail

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
BVHTree<TIndex, TPayload>::BVHTree(BVHTree&& other) noexcept :
    m_root(std::exchange(other.m_root, nullptr))
{
}

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
BVHTree<TIndex, TPayload>& BVHTree<TIndex, TPayload>::operator=(BVHTree&& other) noexcept
{
   if (this == &other)
      return *this;
   m_root = std::exchange(other.m_root, nullptr);

   return *this;
}

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
void BVHTree<TIndex, TPayload>::add(const TPayload& payload)
{
   auto* node = detail::insert_node(m_root, payload);
   m_leave_mapping[payload.index()] = node;
}

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
void BVHTree<TIndex, TPayload>::remove(TIndex index)
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

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
void BVHTree<TIndex, TPayload>::update(const TPayload& payload)
{
   const auto leaf_it = m_leave_mapping.find(payload.index());
   if (leaf_it == m_leave_mapping.end()) {
      return;
   }

   auto* leaf = leaf_it->second;
   leaf->payload = payload;

   detail::update_parent_bounds(leaf->parent);
}

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
BVHTree<TIndex, TPayload>::~BVHTree()
{
   this->clear();
}

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
void BVHTree<TIndex, TPayload>::build(std::span<TPayload> data)
{
   this->clear();
   m_root = detail::build_node<TIndex, TPayload>(m_leave_mapping, data, nullptr);
}

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
void BVHTree<TIndex, TPayload>::clear()
{
   if (m_root != nullptr) {
      detail::delete_node(m_root);
   }
}

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
BVHHit<TIndex, TPayload> BVHTree<TIndex, TPayload>::traverse(const Ray& ray) const
{
   if (m_root == nullptr) {
      return {INFINITY, nullptr};
   }
   return detail::traverse_node(m_root, ray);
}
template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
const TPayload& BVHTree<TIndex, TPayload>::get(TIndex index)
{
   return std::get<TPayload>(m_leave_mapping.at(index)->payload);
}

}// namespace triglav::geometry
