#pragma once

#include "triglav/Math.hpp"

#include <numeric>

namespace triglav::geometry {

namespace detail {

constexpr Axis next_axis(const Axis axis)
{
   switch (axis) {
   case Axis::X:
      return Axis::Y;
   case Axis::Y:
      return Axis::Z;
   case Axis::Z:
      return Axis::X;
   case Axis::W:
      return Axis::W;
   }
   return {};
}

template<PayloadWithAABB TPayload>
void delete_node(const BVHNode<TPayload>* node)
{
   if (node->left != nullptr) {
      delete_node(node->left);
   }
   if (node->right != nullptr) {
      delete_node(node->right);
   }
   delete node;
}

template<PayloadWithAABB TPayload>
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

template<PayloadWithAABB TPayload>
BVHNode<TPayload>* build_node(std::span<TPayload> data, const Axis axis)
{
   if (data.empty())
      return nullptr;
   if (data.size() == 1) {
      return new BVHNode<TPayload>{
         .payload = std::move(data[0]),
         .left = nullptr,
         .right = nullptr,
      };
   }

   std::sort(data.begin(), data.end(), [axis](const TPayload& left, const TPayload& right) {
      return vector3_component(left.bounding_box().centroid(), axis) > vector3_component(right.bounding_box().centroid(), axis);
   });

   const auto mid = data.size() / 2;

   auto* node = new BVHNode<TPayload>{
      .payload = calculate_bounding_box(data),
   };
   node->left = build_node(data.subspan(0, mid), detail::next_axis(axis));
   node->right = build_node(data.subspan(mid, data.size() - mid), next_axis(axis));

   return node;
}

template<PayloadWithAABB TPayload>
BVHNode<TPayload>* traverse_node(BVHNode<TPayload>* node, const Ray& ray)
{
   if (std::holds_alternative<TPayload>(node->payload)) {
      auto& payload = std::get<TPayload>(node->payload);
      auto bb = payload.bounding_box();
      if (!bb.intersect(ray)) {
         return nullptr;
      }
      return node;
   }

   const auto& bb = std::get<BoundingBox>(node->payload);
   if (!bb.intersect(ray)) {
      return nullptr;
   }

   if (auto* left_node = traverse_node(node->left, ray); left_node != nullptr) {
      return left_node;
   }

   return traverse_node(node->right, ray);
}

}// namespace detail

template<PayloadWithAABB TPayload>
BVHTree<TPayload>::BVHTree(BVHTree&& other) noexcept :
    m_root(other.m_root)
{
}

template<PayloadWithAABB TPayload>
BVHTree<TPayload>& BVHTree<TPayload>::operator=(BVHTree&& other) noexcept
{
   if (this == &other)
      return *this;
   m_root = other.m_root;
   return *this;
}

template<PayloadWithAABB TPayload>
BVHTree<TPayload>::~BVHTree()
{
   this->clear();
}

template<PayloadWithAABB TPayload>
void BVHTree<TPayload>::build(std::span<TPayload> data)
{
   this->clear();
   m_root = detail::build_node<TPayload>(data, Axis::X);
}

template<PayloadWithAABB TPayload>
void BVHTree<TPayload>::clear()
{
   if (m_root != nullptr) {
      detail::delete_node(m_root);
   }
}

template<PayloadWithAABB TPayload>
TPayload* BVHTree<TPayload>::traverse(const Ray& ray) const
{
   auto* node = detail::traverse_node(m_root, ray);
   if (node == nullptr)
      return nullptr;
   return &std::get<TPayload>(node->payload);
}

}// namespace triglav::geometry
