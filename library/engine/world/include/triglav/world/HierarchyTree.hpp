#pragma once

#include "World.hpp"

#include <map>

namespace triglav::world {


class HierarchyTree
{
   struct Node
   {
      Node* parent{};
      Node* child{};
      Node* sibling{};
      EntityID entity_id = NO_ENTITY;
   };

 public:
   struct Iterator
   {
      using iterator_category = std::forward_iterator_tag;
      using value_type = EntityID;
      using difference_type = std::ptrdiff_t;
      using pointer = value_type*;
      using reference = value_type&;

      Node* at;

      [[nodiscard]] EntityID operator*() const;
      Iterator& operator++();
      [[nodiscard]] bool operator!=(const Iterator& other) const;
   };

   struct Range
   {
      Iterator from;
      Iterator to;

      [[nodiscard]] Iterator begin() const;
      [[nodiscard]] Iterator end() const;
   };

   HierarchyTree();
   ~HierarchyTree();

   TG_DELETE_ALL(HierarchyTree)

   void add_child(EntityID parent, EntityID child);
   void remove_entity(EntityID entity);
   void remove_tree(EntityID entity);
   [[nodiscard]] Range children_of(EntityID entity) const;
   [[nodiscard]] EntityID parent_of(EntityID entity) const;

 private:
   std::map<EntityID, Node*> m_nodes;
};

}// namespace triglav::world