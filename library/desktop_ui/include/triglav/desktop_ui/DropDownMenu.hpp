#pragma once

#include "DesktopUI.hpp"

#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/widget/Button.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"
#include "triglav/ui_core/widget/TextBox.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

namespace triglav::desktop_ui {

class Dialog;
class DropDownMenu;
class DropDownSelector;

class DropDownSelectorButton final : public ui_core::IWidget
{
 public:
   using Self = DropDownSelectorButton;

   struct State
   {
      String label;
      u32 index;
      DropDownMenu* menu;
   };

   DropDownSelectorButton(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;
   void on_child_state_changed(IWidget& widget) override;

   void on_click(desktop::MouseButton button) const;
   void on_enter() const;
   void on_leave() const;

 private:
   State m_state;
   ui_core::IWidget* m_parent;
   ui_core::Button m_button;
   ui_core::RectBox* m_rect;

   TG_SINK(ui_core::Button, OnClick);
   TG_SINK(ui_core::Button, OnEnter);
   TG_SINK(ui_core::Button, OnLeave);
};

class DropDownSelector final : public ui_core::IWidget
{
   friend class DropDownSelectorButton;

 public:
   using Self = DropDownSelector;

   struct State
   {
      DropDownMenu* menu;
   };

   DropDownSelector(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;
   void on_child_state_changed(IWidget& widget) override;

 private:
   State m_state;
   ui_core::IWidget* m_parent;
   ui_core::VerticalLayout m_verticalLayout;
};

class DropDownMenu final : public ui_core::IWidget
{
   friend class DropDownSelectorButton;
   friend class DropDownSelector;

 public:
   using Self = DropDownMenu;

   struct State
   {
      DesktopUIManager* manager;
      std::vector<String> items;
      u32 selectedItem;
   };

   DropDownMenu(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;
   void on_child_state_changed(IWidget& widget) override;

   void set_selected_item(u32 index);

 private:
   [[maybe_unused]] ui_core::Context& m_context;
   State m_state;

   [[maybe_unused]] ui_core::IWidget* m_parent;
   ui_core::RectBox m_rect;
   ui_core::TextBox* m_label{};
   ui_core::SpriteId m_downArrow{};
   Vector4 m_dimensions;
   Vector4 m_croppingMask;
   Dialog* m_currentPopup{};
};

}// namespace triglav::desktop_ui
