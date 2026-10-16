#pragma once

#include <cassert>
#include <concepts>

namespace triglav::geometry {

template<typename T>
concept BVHNode = requires(T* node) {
   { node->left } -> std::convertible_to<T*>;
   { node->right } -> std::convertible_to<T*>;
   { node->is_leaf() } -> std::same_as<bool>;
};

template<typename T>
concept BVHResult = requires(const T& t) {
   { T::miss() } -> std::same_as<T>;
   { t.score() } -> std::same_as<float>;
};

template<typename T>
concept BVHIntersectionResult = requires(T result) {
   { result.has_hit() } -> std::same_as<bool>;
};

template<typename T>
concept BVHBaseTraits = BVHNode<typename T::Node> && requires(typename T::Node* node, typename T::UserData* user_data) {
   { T::move_payload(user_data, node) } -> std::same_as<typename T::Node*>;
   { T::calculate_insert_cost(user_data, node, node) } -> std::same_as<float>;
   { T::recalculate_bounds(user_data, node) } -> std::same_as<void>;
};

template<BVHBaseTraits TTraits>
struct BVHUtil
{
   using Node = TTraits::Node;
   using UserData = TTraits::UserData;

   static void insert_node(UserData* user_data, Node* parent, Node* node)
   {
      if (parent->is_leaf()) {
         Node* new_leaf = TTraits::move_payload(user_data, parent);

         parent->left = new_leaf;
         parent->right = node;

         TTraits::recalculate_bounds(user_data, parent);

         assert(parent->left->is_leaf());
         assert(parent->right->is_leaf());
         assert(!parent->is_leaf());
         return;
      }

      const auto left_cost = TTraits::calculate_insert_cost(user_data, parent->left, node);
      const auto right_cost = TTraits::calculate_insert_cost(user_data, parent->right, node);

      if (left_cost < right_cost) {
         insert_node(user_data, parent->left, node);
      } else {
         insert_node(user_data, parent->right, node);
      }

      TTraits::recalculate_bounds(user_data, parent);
   }

   static void destroy(Node* node)
   {
      if (node->left != nullptr) {
         destroy(node->left);
      }
      if (node->right != nullptr) {
         destroy(node->right);
      }

      delete node;
   }
};

template<typename T>
concept BVHTraverseTraits =
   BVHBaseTraits<typename T::BaseTraits> && BVHResult<typename T::Result> && BVHIntersectionResult<typename T::IntersectionResult> &&
   requires(const T::Node* node, const T::Traversary& traversary, const T::IntersectionResult& inter_res, float& best_score) {
      { T::test_leaf(node, traversary, best_score) } -> std::same_as<typename T::Result>;
      { T::test_intersection(node, traversary) } -> std::same_as<typename T::IntersectionResult>;
      { T::test_hit_score(inter_res, node, traversary) } -> std::same_as<float>;
   };

template<BVHTraverseTraits TTraits>
struct BVHTraverser
{
   using Result = TTraits::Result;
   using Node = TTraits::BaseTraits::Node;
   using Traversary = TTraits::Traversary;
   using IntersectionResult = TTraits::IntersectionResult;

   static Result traverse(const Node* node, const Traversary& traversary, float& best_score)
   {
      if (node->is_leaf()) {
         return TTraits::test_leaf(node, traversary, best_score);
      }

      const auto left_check = TTraits::test_intersection(node->left, traversary);
      const auto right_check = TTraits::test_intersection(node->right, traversary);

      if (!left_check.has_hit() && !right_check.has_hit()) {
         return Result::miss();
      }
      if (left_check.has_hit() && right_check.has_hit()) {
         const float left_score = TTraits::test_hit_score(left_check, node->left, traversary);
         const float right_score = TTraits::test_hit_score(right_check, node->right, traversary);

         const Node* first_node;
         float first_score;
         const Node* second_node;
         float second_score;

         if (left_score < right_score) {
            first_node = node->left;
            first_score = left_score;
            second_node = node->right;
            second_score = right_score;
         } else {
            first_node = node->right;
            first_score = right_score;
            second_node = node->left;
            second_score = left_score;
         }

         Result first_result = Result::miss();
         if (first_score < best_score) {
            first_result = traverse(first_node, traversary, best_score);
         }
         if (second_score < best_score) {
            Result second_result = traverse(second_node, traversary, best_score);
            if (second_result.score() < first_result.score()) {
               return second_result;
            }
         }
         return first_result;
      }

      Node* child_node{};
      const IntersectionResult* check = {};
      if (left_check.has_hit()) {
         child_node = node->left;
         check = &left_check;
      } else {
         child_node = node->right;
         check = &right_check;
      }

      const float score = TTraits::test_hit_score(*check, child_node, traversary);
      if (score >= best_score)
         return Result::miss();

      return traverse(child_node, traversary, best_score);
   }
};

}// namespace triglav::geometry
