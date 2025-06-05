#include "TextInput.hpp"

#include "triglav/desktop/ISurface.hpp"
#include "triglav/ui_core/UICore.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"
#include "triglav/ui_core/widget/TextBox.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

#include <spdlog/spdlog.h>

namespace triglav::desktop_ui {

TextInput::TextInput(ui_core::Context& ctx, const TextInput::State state, ui_core::IWidget* /*parent*/) :
    m_state(state),
    m_rect(ctx,
           {
              .color = m_state.manager->properties().button_bg_color,
              .borderRadius = {10.0f, 10.0f, 10.0f, 10.0f},
              .borderColor = {0.1f, 0.1f, 0.1f, 1.f},
              .borderWidth = 2.5f,
           },
           this)
{
   const auto& props = m_state.manager->properties();
   auto& layout = m_rect.create_content<ui_core::VerticalLayout>({
      .padding{15.0f, 15.0f, 15.0f, 15.0f},
      .separation = 0.0f,
   });
   m_text = &layout.create_child<ui_core::TextBox>({
      .fontSize = props.button_font_size,
      .typeface = props.base_typeface,
      .content = state.text.data(),
      .color = props.foreground_color,
      .horizontalAlignment = ui_core::HorizontalAlignment::Center,
      .verticalAlignment = ui_core::VerticalAlignment::Center,
   });
}

Vector2 TextInput::desired_size(const Vector2 parentSize) const
{
   return m_rect.desired_size(parentSize);
}

void TextInput::add_to_viewport(const Vector4 dimensions)
{
   m_rect.add_to_viewport(dimensions);
}

void TextInput::remove_from_viewport()
{
   m_rect.remove_from_viewport();
}

void TextInput::on_event(const ui_core::Event& event)
{
   switch (event.eventType) {
   case ui_core::Event::Type::MousePressed:
      m_rect.set_color(m_state.manager->properties().button_bg_pressed_color);
      break;
   case ui_core::Event::Type::MouseReleased:
      m_rect.set_color(m_state.manager->properties().button_bg_hover_color);
      break;
   case ui_core::Event::Type::MouseEntered:
      m_state.manager->surface().set_cursor_icon(desktop::CursorIcon::Edit);
      m_state.manager->surface().set_keyboard_input_mode(desktop::KeyboardInputMode::Text);
      m_rect.set_color(m_state.manager->properties().button_bg_hover_color);
      m_isActive = true;
      break;
   case ui_core::Event::Type::MouseLeft:
      m_state.manager->surface().set_cursor_icon(desktop::CursorIcon::Arrow);
      m_state.manager->surface().set_keyboard_input_mode(desktop::KeyboardInputMode::Direct);
      m_rect.set_color(m_state.manager->properties().button_bg_color);
      m_isActive = false;
      break;
   case ui_core::Event::Type::TextInput: {
      const auto rune = std::get<ui_core::Event::TextInput>(event.data).inputRune;
      if (rune == '\b') {
         if (!m_state.text.is_empty()) {
            m_state.text.shrink_by(1);
         }
      } else {
         m_state.text.append_rune(rune);
      }
      m_text->set_content({m_state.text.data(), m_state.text.size()});
      break;
   }
   default:
      break;
   }
}

}// namespace triglav::desktop_ui
