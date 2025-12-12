#include "TreeController.hpp"

namespace triglav::desktop_ui {

TreeItemId TreeController::add_item(const TreeItemId parent, const TreeItem& item)
{
   const auto item_id = m_top_id++;
   m_items[item_id] = item;
   m_hierarchy[parent].push_back(item_id);
   m_parent[item_id] = parent;
   return item_id;
}

const std::vector<TreeItemId>& TreeController::children(const TreeItemId parent)
{
   return m_hierarchy.at(parent);
}

const TreeItem& TreeController::item(const TreeItemId id)
{
   return m_items.at(id);
}

void TreeController::set_label(const TreeItemId id, const StringView label)
{
   m_items.at(id).label = label;
}

void TreeController::remove(const TreeItemId id)
{
   const auto parent = m_parent.at(id);
   if (m_hierarchy.contains(id)) {
      for (const auto child : m_hierarchy.at(id)) {
         this->remove(child);
      }
      m_hierarchy.erase(id);
   }

   auto& parent_hi = m_hierarchy.at(parent);
   parent_hi.erase(std::ranges::find(parent_hi, id));

   m_parent.erase(id);
   m_items.erase(id);
}

}// namespace triglav::desktop_ui
