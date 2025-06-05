#pragma once

#include "DesktopUI.hpp"

#include "triglav/String.hpp"
#include "triglav/event/Delegate.hpp"
#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"

namespace triglav::ui_core {
class TextBox;
class RectBox;
}// namespace triglav::ui_core
namespace triglav::desktop_ui {

class TextInput final : public ui_core::IWidget
{
 public:
   using Self = TextInput;

   struct State
   {
      DesktopUIManager* manager;
      String text;
   };

   TextInput(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;

 private:
   State m_state;
   ui_core::RectBox m_rect;
   ui_core::TextBox* m_text{};
   bool m_isActive{false};
};

}// namespace triglav::desktop_ui
