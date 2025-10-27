#pragma once

#include "Geometry.hpp"

#include <variant>

namespace triglav::geometry {

template<typename T>
concept PayloadWithAABB = requires(T t) {
   { t.bounding_box() } -> std::convertible_to<BoundingBox>;
};

template<PayloadWithAABB TPayload>
struct BVHNode
{
   std::variant<TPayload, BoundingBox> payload;
   BVHNode* left;
   BVHNode* right;
};

template<PayloadWithAABB TPayload>
class BVHTree
{
 public:
   BVHTree() = default;
   ~BVHTree();

   BVHTree(const BVHTree& other) = delete;
   BVHTree& operator=(const BVHTree& other) = delete;

   BVHTree(BVHTree&& other) noexcept;
   BVHTree& operator=(BVHTree&& other) noexcept;

   void build(std::span<TPayload> data);
   void clear();
   TPayload* traverse(const Ray& ray) const;

 private:
   BVHNode<TPayload>* m_root{};
};

}// namespace triglav::geometry

#include "BVHTree.inl"