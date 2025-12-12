#pragma once

#include "DesktopUI.hpp"

#include "triglav/ui_core/PrimitiveHelpers.hpp"
#include "triglav/ui_core/widget/Button.hpp"

namespace triglav::ui_core {
class TextBox;
class RectBox;
}// namespace triglav::ui_core
namespace triglav::desktop_ui {

class Button final : public ui_core::ContainerWidget
{
 public:
   using Self = Button;

   TG_EVENT(OnClick)

   struct State
   {
      DesktopUIManager* manager;
   };

   Button(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parent_size) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;

 private:
   State m_state;
   ui_core::RectInstance m_background;
};

}// namespace triglav::desktop_ui
