#pragma once

#include "Geometry.hpp"

#include <map>
#include <variant>

namespace triglav::geometry {

template<typename T, typename TIndex>
concept TopLevelPrimitive = requires(T t) {
   { t.bounding_box() } -> std::convertible_to<BoundingBox>;
   { t.index() } -> std::convertible_to<TIndex>;
   { t.inv_transform_matrix() } -> std::convertible_to<Matrix4x4>;
};

template<typename T, typename TIndex, typename TBottomLevel>
concept BottomLevelProvider = requires(T t, TIndex index) {
   { t.get_bottom_level(index) } -> std::convertible_to<TBottomLevel&>;
};

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload>
struct TopLevelNode
{
   bool is_leaf_node = true;
   TopLevelNode* parent{};

   union
   {
      struct
      {
         float max_surface_area{};
         BoundingBox bounding_box{};
         TopLevelNode* left{};
         TopLevelNode* right{};
      } internal{};
      TPayload leaf;
   };

   [[nodiscard]] bool is_leaf() const
   {
      return this->is_leaf_node;
   }
};

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload>
struct TopLevelHit
{
   float distance;
   const TPayload* payload;

   static TopLevelHit miss()
   {
      return {INF, nullptr};
   }

   [[nodiscard]] float score() const
   {
      return distance;
   }
};

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload>
struct TopLevelSweptHit
{
   float time{INF};
   Vector3 normal{};
   SweepStatus status{};
   const TPayload* payload{nullptr};

   static TopLevelSweptHit miss()
   {
      return {};
   }

   [[nodiscard]] float score() const
   {
      return time;
   }
};

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload, typename TBottomLevel,
         BottomLevelProvider<TIndex, TBottomLevel> TBottomLevelProvider>
struct TopLevelBaseTraits;

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload, typename TBottomLevel,
         BottomLevelProvider<TIndex, TBottomLevel> TBottomLevelProvider>
class TopLevelBVH
{
   friend struct TopLevelBaseTraits<TIndex, TPayload, TBottomLevel, TBottomLevelProvider>;

 public:
   explicit TopLevelBVH(TBottomLevelProvider& provider);
   ~TopLevelBVH();

   TopLevelBVH(const TopLevelBVH& other) = delete;
   TopLevelBVH& operator=(const TopLevelBVH& other) = delete;

   TopLevelBVH(TopLevelBVH&& other) noexcept;
   TopLevelBVH& operator=(TopLevelBVH&& other) noexcept;

   void add(const TPayload& payload);
   void remove(TIndex index);
   void update(const TPayload& payload);
   void build(std::span<TPayload> data);
   void clear();
   TopLevelHit<TIndex, TPayload> traverse(const Ray& ray) const;

   TopLevelSweptHit<TIndex, TPayload> traverse_swept(const BoundingBox& bb, const Vector3& movement) const;

   const TPayload& get(TIndex index);

 private:
   TopLevelNode<TIndex, TPayload>* m_root{};
   std::map<TIndex, TopLevelNode<TIndex, TPayload>*> m_leave_mapping;
   TBottomLevelProvider& m_bl_provider;
};

}// namespace triglav::geometry

#include "BVHTree.inl"