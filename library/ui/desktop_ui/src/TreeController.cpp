#include "TreeController.hpp"

namespace triglav::desktop_ui {

TreeItemId TreeController::add_item(const TreeItemId parent, const TreeItem& item)
{
   const auto item_id = m_top_id++;
   m_items[item_id] = item;
   m_hierarchy[parent].push_back(item_id);
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

}// namespace triglav::desktop_ui
