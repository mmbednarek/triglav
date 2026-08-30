#include "HierarchyTree.hpp"

#include <ranges>
#include <queue>

namespace triglav::world {

EntityID HierarchyTree::Iterator::operator*() const
{
   return at->entity_id;
}

HierarchyTree::Iterator& HierarchyTree::Iterator::operator++()
{
   if (at != nullptr) {
      at = at->sibling;
   }
   return *this;
}

bool HierarchyTree::Iterator::operator!=(const Iterator& other) const
{
   return at != other.at;
}

HierarchyTree::Iterator HierarchyTree::Range::begin() const
{
   return from;
}

HierarchyTree::Iterator HierarchyTree::Range::end() const
{
   return to;
}

HierarchyTree::HierarchyTree()
{
   auto* node = new Node{};
   node->entity_id = 0;
   node->sibling = nullptr;
   node->parent = nullptr;
   m_nodes[0] = node;
}

HierarchyTree::~HierarchyTree()
{
   for (const auto& node : std::views::values(m_nodes)) {
      delete node;
   }
}

void HierarchyTree::add_child(const EntityID parent, const EntityID child)
{
   auto* parent_node = m_nodes.at(parent);
   auto* node = new Node{};

   node->parent = parent_node;
   node->entity_id = child;
   node->sibling = parent_node->child;
   parent_node->child = node;

   m_nodes[child] = node;
}

void HierarchyTree::remove_entity(const EntityID entity)
{
   assert(m_nodes.contains(entity));
   Node* node = m_nodes.at(entity);

   Node** pointer_to_node = &node->parent->child;
   while (*pointer_to_node != node) {
      pointer_to_node = &(*pointer_to_node)->sibling;
   }

   if (node->child != nullptr) {
      Node** child_pointer_to_node = &node->child;
      while (*child_pointer_to_node != nullptr) {
         (*child_pointer_to_node)->parent = node->parent;
         child_pointer_to_node = &(*child_pointer_to_node)->sibling;
      }

      *child_pointer_to_node = node->sibling;
      *pointer_to_node = node->child;
   } else {
      *pointer_to_node = node->sibling;
   }

   m_nodes.erase(entity);
   delete node;
}

void HierarchyTree::remove_tree(const EntityID entity)
{
   Node* root = m_nodes.at(entity);

   std::queue<Node*> removal_queue;
   removal_queue.push(root->child);

   while (!removal_queue.empty()) {
      Node* node = removal_queue.front();
      removal_queue.pop();

      if (node->sibling != nullptr) {
         removal_queue.push(node->sibling);
      }
      if (node->child != nullptr) {
         removal_queue.push(node->child);
      }

      m_nodes.erase(node->entity_id);
      delete node;
   }

   Node** pointer_to_node = &root->parent->child;
   while (*pointer_to_node != root) {
      pointer_to_node = &(*pointer_to_node)->sibling;
   }
   *pointer_to_node = (*pointer_to_node)->sibling;

   m_nodes.erase(root->entity_id);
   delete root;
}

HierarchyTree::Range HierarchyTree::children_of(const EntityID entity) const
{
   return Range{Iterator{m_nodes.at(entity)->child}, Iterator{nullptr}};
}

EntityID HierarchyTree::parent_of(const EntityID entity) const
{
   const Node* parent = m_nodes.at(entity)->parent;
   if (parent == nullptr) {
      return NO_ENTITY;
   }
   return parent->entity_id;
}

}// namespace triglav::world
