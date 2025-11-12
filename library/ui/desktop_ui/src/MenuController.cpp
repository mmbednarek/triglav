#include "MenuController.hpp"

namespace triglav::desktop_ui {

using namespace name_literals;

void MenuController::add_item(const Name name, const StringView label)
{
   this->add_subitem("root"_name, name, label);
}

void MenuController::add_submenu(const Name name, const StringView label)
{
   this->add_submenu("root"_name, name, label);
}

void MenuController::add_submenu(const Name parent, const Name name, const StringView label)
{
   m_hierarchy[parent].push_back(name);
   m_items[name] = {
      .name = name,
      .label = label,
      .isSubmenu = true,
   };
}

void MenuController::add_subitem(const Name parent, const Name name, const StringView label)
{
   m_hierarchy[parent].push_back(name);
   m_items[name] = {
      .name = name,
      .label = label,
      .isSubmenu = false,
   };
}

void MenuController::add_seperator()
{
   m_hierarchy["root"_name].push_back("separator"_name);
}

void MenuController::add_seperator(Name parent)
{
   m_hierarchy[parent].push_back("separator"_name);
}

const std::vector<Name>& MenuController::children(const Name parent) const
{
   return m_hierarchy.at(parent);
}

const MenuItem& MenuController::item(const Name name) const
{
   return m_items.at(name);
}

void MenuController::trigger_on_clicked(const Name name)
{
   event_OnClicked.publish(name, m_items.at(name));
}

}// namespace triglav::desktop_ui
