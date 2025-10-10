#pragma once

#include "DesktopUI.hpp"
#include "MenuList.hpp"

namespace triglav::desktop_ui {

class Dialog;

class ContextMenu final : public ui_core::ContainerWidget
{
 public:
   using Self = ContextMenu;
   struct State
   {
      DesktopUIManager* manager;
      MenuController* controller;
   };

   ContextMenu(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;

   void on_clicked(Name name, const MenuItem& item);

 private:
   State m_state;
   std::unique_ptr<MenuList> m_menu;
   Dialog* m_menuDialog = nullptr;

   TG_SINK(MenuController, OnClicked);
};

}// namespace triglav::desktop_ui
