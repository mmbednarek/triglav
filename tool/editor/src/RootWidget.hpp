#pragma once

#include "triglav/desktop_ui/DesktopUI.hpp"
#include "triglav/desktop_ui/MenuController.hpp"
#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

namespace triglav::desktop_ui {
class MenuBar;
}

namespace triglav::editor {

class Editor;
class LevelEditor;

class RootWidget final : public ui_core::ProxyWidget
{
 public:
   using Self = RootWidget;
   struct State
   {
      desktop_ui::PopupManager* dialog_manager;
      Editor* editor;
   };

   RootWidget(ui_core::Context& context, State state, ui_core::IWidget* parent);

   void on_clicked_menu_bar(Name item_name, const desktop_ui::MenuItem& item) const;
   void tick(float delta_time) const;

 private:
   State m_state;
   desktop_ui::DesktopUIManager m_desktop_uimanager;
   desktop_ui::MenuController m_menu_bar_controller;
   desktop_ui::MenuBar* m_menu_bar;
   LevelEditor* m_level_editor;

   TG_SINK(desktop_ui::MenuController, OnClicked);
};

}// namespace triglav::editor