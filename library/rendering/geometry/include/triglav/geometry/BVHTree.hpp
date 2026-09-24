#pragma once

#include "Geometry.hpp"

#include <map>
#include <variant>

namespace triglav::geometry {

template<typename T, typename TIndex>
concept PayloadWithAABB = requires(T t) {
   { t.bounding_box() } -> std::convertible_to<BoundingBox>;
   { t.index() } -> std::convertible_to<TIndex>;
};

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
struct BVHNode
{
   std::variant<TPayload, BoundingBox> payload;
   BVHNode* parent;
   BVHNode* left;
   BVHNode* right;
};

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
struct BVHHit
{
   float distance;
   TPayload* payload;
};

template<typename TIndex, PayloadWithAABB<TIndex> TPayload>
class BVHTree
{
 public:
   BVHTree() = default;
   ~BVHTree();

   BVHTree(const BVHTree& other) = delete;
   BVHTree& operator=(const BVHTree& other) = delete;

   BVHTree(BVHTree&& other) noexcept;
   BVHTree& operator=(BVHTree&& other) noexcept;

   void add(const TPayload& payload);
   void remove(TIndex index);
   void update(const TPayload& payload);
   void build(std::span<TPayload> data);
   void clear();
   BVHHit<TIndex, TPayload> traverse(const Ray& ray) const;

   const TPayload& get(TIndex index);

 private:
   BVHNode<TIndex, TPayload>* m_root{};
   std::map<TIndex, BVHNode<TIndex, TPayload>*> m_leave_mapping;
};

}// namespace triglav::geometry

#include "BVHTree.inl"