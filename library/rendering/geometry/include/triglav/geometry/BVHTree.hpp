#pragma once

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
};

struct BottomLevelNode
{
   u32 primitive_id;
   BoundingBox bounding_box;
   BottomLevelNode* left;
   BottomLevelNode* right;
   std::array<Vector3, 3> primitive;
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
   template<typename TCallback>
   void traverse_aabb(const BoundingBox& bb, TCallback callback) const;
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
};

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload>
struct TopLevelHit
{
   float distance;
   TPayload* payload;
};

template<typename TIndex, TopLevelPrimitive<TIndex> TPayload, typename TBottomLevel,
         BottomLevelProvider<TIndex, TBottomLevel> TBottomLevelProvider>
class TopLevelBVH
{
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

   template<typename TCallback>
   void traverse_aabb(const BoundingBox& bb, TCallback callback) const;

   const TPayload& get(TIndex index);

 private:
   TopLevelNode<TIndex, TPayload>* m_root{};
   std::map<TIndex, TopLevelNode<TIndex, TPayload>*> m_leave_mapping;
   TBottomLevelProvider& m_bl_provider;
};

}// namespace triglav::geometry

#include "BVHTree.inl"