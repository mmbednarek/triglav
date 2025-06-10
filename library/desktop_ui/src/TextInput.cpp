#include "TextInput.hpp"

#include "triglav/desktop/ISurface.hpp"
#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/threading/Scheduler.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/Primitives.hpp"
#include "triglav/ui_core/UICore.hpp"
#include "triglav/ui_core/Viewport.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"
#include "triglav/ui_core/widget/TextBox.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

namespace triglav::desktop_ui {

using namespace std::chrono_literals;

TextInput::TextInput(ui_core::Context& ctx, const TextInput::State state, ui_core::IWidget* /*parent*/) :
    m_context(ctx),
    m_state(state),
    m_rect(ctx,
           {
              .color = m_state.manager->properties().button_bg_color,
              .borderRadius = {10.0f, 10.0f, 10.0f, 10.0f},
              .borderColor = {0.1f, 0.1f, 0.1f, 1.f},
              .borderWidth = 2.5f,
           },
           this),
    m_caretPosition(m_state.text.rune_count())
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
   auto size = m_rect.desired_size(parentSize);
   return {m_state.width, size.y};
}

void TextInput::add_to_viewport(const Vector4 dimensions)
{
   m_rect.add_to_viewport(dimensions);

   m_caretBox = m_context.viewport().add_rectangle(ui_core::Rectangle{
      .rect = {dimensions.x + 15, dimensions.y + 12, dimensions.x + 16, dimensions.y + dimensions.w - 12},
      .color = m_state.manager->properties().foreground_color,
      .borderRadius = {},
      .borderColor = {},
      .borderWidth = 0.0f,
   });

   this->recalculate_caret_offset();

   m_dimensions = dimensions;
}

void TextInput::remove_from_viewport()
{
   m_context.viewport().remove_rectangle(m_caretBox);
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
      m_state.manager->surface().set_keyboard_input_mode(desktop::KeyboardInputMode::Text | desktop::KeyboardInputMode::Direct);
      m_rect.set_color(m_state.manager->properties().button_bg_hover_color);
      m_isActive = true;
      if (!m_timeoutHandle.has_value()) {
         m_timeoutHandle =
            threading::Scheduler::the().register_timeout(std::chrono::milliseconds{500}, [this]() { this->update_carret_state(); });
      }
      break;
   case ui_core::Event::Type::MouseLeft:
      m_state.manager->surface().set_cursor_icon(desktop::CursorIcon::Arrow);
      m_state.manager->surface().set_keyboard_input_mode(desktop::KeyboardInputMode::Direct);
      m_rect.set_color(m_state.manager->properties().button_bg_color);
      m_isActive = false;
      if (m_timeoutHandle.has_value()) {
         threading::Scheduler::the().cancel(*m_timeoutHandle);
         m_timeoutHandle.reset();
      }
      break;
   case ui_core::Event::Type::TextInput: {
      const auto rune = std::get<ui_core::Event::TextInput>(event.data).inputRune;
      if (font::Charset::European.contains(rune)) {
         m_state.text.insert_rune_at(m_caretPosition, rune);
         ++m_caretPosition;
         this->recalculate_caret_offset();
         m_text->set_content(m_state.text.view());
      }
      break;
   }
   case ui_core::Event::Type::KeyPressed: {
      const auto value = std::get<ui_core::Event::Keyboard>(event.data);
      switch (value.key) {
      case desktop::Key::LeftArrow:
         if (m_caretPosition != 0) {
            m_caretPosition--;
         }
         this->recalculate_caret_offset();
         break;
      case desktop::Key::RightArrow:
         if (m_caretPosition != m_state.text.size()) {
            m_caretPosition++;
         }
         this->recalculate_caret_offset();
         break;
      case desktop::Key::Backspace: {
         if (!m_state.text.is_empty()) {
            m_caretPosition--;
            m_state.text.remove_rune_at(m_caretPosition);
            this->recalculate_caret_offset();
            m_text->set_content(m_state.text.view());
         }
         break;
      }
      default:
         break;
      }
   }
   default:
      break;
   }
}

void TextInput::update_carret_state()
{
   m_isCarretVisible = !m_isCarretVisible;
   if (m_isCarretVisible) {
      m_context.viewport().set_rectangle_color(m_caretBox, m_state.manager->properties().foreground_color);
   } else {
      m_context.viewport().set_rectangle_color(m_caretBox, {0, 0, 0, 0});
   }

   const auto duration = m_isCarretVisible ? 400ms : 200ms;
   m_timeoutHandle = threading::Scheduler::the().register_timeout(duration, [this]() { this->update_carret_state(); });
}

void TextInput::recalculate_caret_offset()
{
   float caretOffset = 0.0f;
   const auto runeCount = m_state.text.rune_count();
   if (m_caretPosition >= runeCount) {
      m_caretPosition = runeCount;
   }

   if (m_caretPosition != 0) {
      auto substr = m_state.text.subview(0, static_cast<i32>(m_caretPosition));

      auto& glyphAtlas = m_context.glyph_cache().find_glyph_atlas({m_text->state().typeface, m_text->state().fontSize});
      const auto measure = glyphAtlas.measure_text(substr);

      caretOffset = measure.width;
   }

   m_context.viewport().set_rectangle_dims(m_caretBox, {m_dimensions.x + 15 + caretOffset, m_dimensions.y + 12,
                                                        m_dimensions.x + 16 + caretOffset, m_dimensions.y + m_dimensions.w - 12});
}

}// namespace triglav::desktop_ui
