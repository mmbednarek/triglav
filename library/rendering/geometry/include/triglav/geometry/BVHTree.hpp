#pragma once

#include "BVHUtil.hpp"
#include "Geometry.hpp"
#include "triglav/Macros.hpp"

#include <map>
#include <variant>

namespace triglav::geometry {

template<typename T>
concept TriangleMesh = requires(T t, const u32 index) {
   { t.get_primitive(index) } -> std::convertible_to<std::array<Vector3, 3>>;
   { t.primitive_count() } -> std::convertible_to<u32>;
};

struct BottomLevelHit
{
   float distance;
   u32 primitive_id;

   static BottomLevelHit miss()
   {
      return BottomLevelHit{
         .distance = INF,
         .primitive_id = ~0u,
      };
   }

   [[nodiscard]] float score() const
   {
      return distance;
   }
};

struct BottomLevelNode
{
   u32 primitive_id;
   BoundingBox bounding_box;
   BottomLevelNode* left;
   BottomLevelNode* right;
   std::array<Vector3, 3> primitive;

   [[nodiscard]] bool is_leaf() const
   {
      return primitive_id != ~0u;
   }
};

struct BottomLevelSweepResult
{
   float time{INF};
   Vector3 normal{};
   u32 primitive_id{~0u};

   static BottomLevelSweepResult miss()
   {
      return BottomLevelSweepResult{};
   }

   [[nodiscard]] float score() const
   {
      return time;
   }
};

template<TriangleMesh TMesh>
class BottomLevelBVH
{
 public:
   BottomLevelBVH() = default;
   ~BottomLevelBVH();

   TG_DELETE_COPY(BottomLevelBVH)

   BottomLevelBVH(BottomLevelBVH&& other) noexcept;
   BottomLevelBVH& operator=(BottomLevelBVH&& other) noexcept;

   void build(TMesh& mesh);
   BottomLevelHit traverse(const Ray& ray) const;

   BottomLevelSweepResult traverse_swept(const BoundingBox& bb, const Vector3& movement) const;

   void clear();

 private:
   BottomLevelNode* m_root{};
};

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
   std::variant<TPayload, BoundingBox> payload;
   float max_surface_area;
   TopLevelNode* parent;
   TopLevelNode* left;
   TopLevelNode* right;

   [[nodiscard]] bool is_leaf() const
   {
      return std::holds_alternative<TPayload>(payload);
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