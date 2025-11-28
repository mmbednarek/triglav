#pragma once

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"
#include "triglav/String.hpp"

#include <map>
#include <vector>

namespace triglav::desktop_ui {

using TreeItemId = u32;

constexpr TreeItemId TREE_ROOT = 0;

struct TreeItem
{
   TextureName icon_name;
   Vector4 icon_region;
   String label;
   bool has_children;
};

class ITreeController
{
 public:
   virtual const std::vector<TreeItemId>& children(TreeItemId parent) = 0;
   virtual const TreeItem& item(TreeItemId id) = 0;
   virtual void set_label(TreeItemId id, StringView label) = 0;
   virtual void remove(TreeItemId id) = 0;
};

class TreeController final : public ITreeController
{
 public:
   TreeItemId add_item(TreeItemId parent, const TreeItem& item);

   const std::vector<TreeItemId>& children(TreeItemId parent) override;
   const TreeItem& item(TreeItemId id) override;
   void set_label(TreeItemId id, StringView label) override;
   void remove(TreeItemId id) override;

 private:
   std::map<TreeItemId, std::vector<TreeItemId>> m_hierarchy;
   std::map<TreeItemId, TreeItemId> m_parent;
   std::map<TreeItemId, TreeItem> m_items;
   TreeItemId m_top_id = 1;
};

}// namespace triglav::desktop_ui
