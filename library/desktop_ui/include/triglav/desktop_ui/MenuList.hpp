#pragma once

#include "MenuController.hpp"

#include "triglav/Name.hpp"
#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/Primitives.hpp"

namespace triglav::desktop_ui {

class Dialog;
class DesktopUIManager;

class MenuList final : public ui_core::BaseWidget
{
 public:
   struct State
   {
      DesktopUIManager* manager;
      MenuController* controller;
      Name listName;
      Vector2 screenOffset;
   };

   MenuList(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;

   static Vector2 calculate_size(const MenuController& controller, Name list_name);

 private:
   [[nodiscard]] u32 index_from_mouse_position(Vector2 position) const;

   ui_core::Context& m_context;
   State m_state;
   std::vector<ui_core::TextId> m_labels;
   ui_core::RectId m_backgroundId;
   ui_core::RectId m_hoverRectId{};
   Vector4 m_croppingMask;
   u32 m_hoverIndex = ~0;
   Dialog* m_subMenu = nullptr;
   float m_sizePerItem{};
};

}// namespace triglav::desktop_ui