#pragma once

#include "DesktopUI.hpp"

#include "triglav/ui_core/widget/Button.hpp"

namespace triglav::ui_core {
class TextBox;
class RectBox;
}// namespace triglav::ui_core
namespace triglav::desktop_ui {

class Button final : public ui_core::IWidget
{
 public:
   using Self = Button;

   TG_EVENT(OnClick, desktop::MouseButton)

   struct State
   {
      DesktopUIManager* manager;
      String label;
   };

   Button(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parent_size) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;

 private:
   State m_state;
   ui_core::Button m_button;
   ui_core::RectBox* m_rect{};
   ui_core::TextBox* m_label{};
};

}// namespace triglav::desktop_ui
