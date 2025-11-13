#pragma once

#include "DesktopUI.hpp"

#include "triglav/String.hpp"
#include "triglav/event/Delegate.hpp"
#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/PrimitiveHelpers.hpp"
#include "triglav/ui_core/Primitives.hpp"
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

   TG_EVENT(OnSubmit)

   struct State
   {
      DesktopUIManager* manager;
      String text;
      std::function<bool(Rune)> filter_func = [](Rune) { return true; };
      Color border_color = {0.1f, 0.1f, 0.1f, 1.0f};
   };

   TextInput(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;
   void update_carret_state();
   void set_content(StringView content);
   [[nodiscard]] const String& content() const;

   void on_mouse_pressed(const ui_core::Event& event, const ui_core::Event::Mouse& mouse);
   void on_mouse_entered(const ui_core::Event& event);
   void on_mouse_left(const ui_core::Event& event);
   void on_text_input(const ui_core::Event& event, const ui_core::Event::TextInput& text_input);
   void on_key_pressed(const ui_core::Event& event, const ui_core::Event::Keyboard& keyboard);

 private:
   void recalculate_caret_offset(bool removal = false);
   void update_text_position();

   ui_core::Context& m_context;
   State m_state;

   ui_core::RectInstance m_backgroundRect;
   ui_core::TextInstance m_textPrim;
   ui_core::RectInstance m_caretBox;
   bool m_isCarretVisible{false};
   u32 m_caretPosition{};
   Vector4 m_dimensions{};
   Vector4 m_croppingMask{};
   Vector2 m_textSize{};
   float m_textXPosition{};
   float m_textOffset{};
   float m_caretOffset{};

   bool m_isActive{false};
   std::optional<u32> m_timeoutHandle{};
};


}// namespace triglav::desktop_ui
