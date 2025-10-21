#pragma once

#include "triglav/desktop_ui/DesktopUI.hpp"
#include "triglav/desktop_ui/TreeController.hpp"
#include "triglav/ui_core/IWidget.hpp"

#include <map>
#include <string>

namespace triglav::editor {

class ProjectExplorer : public ui_core::ProxyWidget
{
 public:
   using Self = ProjectExplorer;
   struct State
   {
      desktop_ui::DesktopUIManager* manager;
   };

   ProjectExplorer(ui_core::Context& context, State state, ui_core::IWidget* parent);

 private:
   void init_controller();
   void add_controller_item(std::string_view path);

   State m_state;
   desktop_ui::TreeController m_controller;
   std::map<std::string_view, u32> m_pathToId;
};

}// namespace triglav::editor
