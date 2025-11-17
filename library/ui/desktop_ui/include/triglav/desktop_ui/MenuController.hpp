#pragma once

#include "triglav/Name.hpp"
#include "triglav/String.hpp"
#include "triglav/event/Delegate.hpp"

#include <map>
#include <vector>

namespace triglav::desktop_ui {

struct MenuItem
{
   Name name;
   String label;
   bool is_submenu;
};

class MenuController
{
 public:
   using Self = MenuController;

   TG_EVENT(OnClicked, Name, const MenuItem&)

   void add_item(Name name, StringView label);
   void add_submenu(Name name, StringView label);
   void add_submenu(Name parent, Name name, StringView label);
   void add_subitem(Name parent, Name name, StringView label);
   void add_seperator();
   void add_seperator(Name parent);

   const std::vector<Name>& children(Name parent) const;
   const MenuItem& item(Name name) const;

   void trigger_on_clicked(Name name);

 private:
   std::map<Name, MenuItem> m_items;
   std::map<Name, std::vector<Name>> m_hierarchy;
};

}// namespace triglav::desktop_ui
