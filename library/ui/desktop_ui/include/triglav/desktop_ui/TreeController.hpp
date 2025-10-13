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
   TextureName iconName;
   Vector4 iconRegion;
   String label;
   bool hasChildren;
};

class ITreeController
{
 public:
   virtual const std::vector<TreeItemId>& children(TreeItemId parent) = 0;
   virtual const TreeItem& item(TreeItemId id) = 0;
};

class TreeController final : public ITreeController
{
 public:
   TreeItemId add_item(TreeItemId parent, const TreeItem& item);

   const std::vector<TreeItemId>& children(TreeItemId parent) override;
   const TreeItem& item(TreeItemId id) override;

 private:
   std::map<TreeItemId, std::vector<TreeItemId>> m_hierarchy;
   std::map<TreeItemId, TreeItem> m_items;
   TreeItemId m_topId = 1;
};

}// namespace triglav::desktop_ui
