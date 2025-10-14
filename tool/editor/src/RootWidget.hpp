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

class RootWidget : public ui_core::BaseWidget
{
 public:
   using Self = RootWidget;
   struct State
   {
      desktop_ui::DialogManager* dialogManager;
      Editor* editor;
   };

   RootWidget(ui_core::Context& context, State state, ui_core::IWidget* parent);

   Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event);

   void on_clicked_menu_bar(Name itemName, const desktop_ui::MenuItem& item);

 private:
   ui_core::Context& m_context;
   State m_state;
   ui_core::VerticalLayout m_globalLayout;
   desktop_ui::DesktopUIManager m_deskopUIManager;
   desktop_ui::MenuController m_menuBarController;
   desktop_ui::MenuBar* m_menuBar;

   TG_SINK(desktop_ui::MenuController, OnClicked);
};

}// namespace triglav::editor