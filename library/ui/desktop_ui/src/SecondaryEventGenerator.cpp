#include "SecondaryEventGenerator.hpp"

#include "triglav/ui_core/Context.hpp"

namespace triglav::desktop_ui {

SecondaryEventGenerator::SecondaryEventGenerator(ui_core::Context& ctx, ui_core::IWidget* parent) :
    ui_core::ProxyWidget(ctx, parent)
{
}

void SecondaryEventGenerator::on_event(const ui_core::Event& event)
{
   ui_core::visit_event<void>(*this, event);

   if (m_context.active_widget() != nullptr) {
      if (event.event_type == ui_core::Event::Type::TextInput || event.event_type == ui_core::Event::Type::KeyPressed ||
          event.event_type == ui_core::Event::Type::MouseMoved) {
         ui_core::Event sub_event(event);
         sub_event.mouse_position = event.global_mouse_position - rect_position(m_context.active_area());
         sub_event.is_forwarded_to_active = true;
         m_context.active_widget()->on_event(sub_event);
      }
   }

   ProxyWidget::on_event(event);
}

void SecondaryEventGenerator::on_mouse_pressed(const ui_core::Event& event, const ui_core::Event::Mouse& /*mouse*/) const
{
   if (m_context.active_widget() != nullptr) {
      if (!is_point_inside(m_context.active_area(), event.global_mouse_position)) {
         m_context.set_active_widget(nullptr, {});
      }
   }
}

static std::optional<Modifier> key_to_modifier(const desktop::Key key)
{
   switch (key) {
   case desktop::Key::Control:
      return Modifier::Control;
   case desktop::Key::Shift:
      return Modifier::Shift;
   case desktop::Key::Alt:
      return Modifier::Alt;
   default:
      break;
   }
   return std::nullopt;
}

void SecondaryEventGenerator::on_key_pressed(const ui_core::Event& event, const ui_core::Event::Keyboard& keyboard)
{
   if (const auto mod = key_to_modifier(keyboard.key); mod.has_value()) {
      m_modifier_state |= *mod;
      return;
   }

   if (m_modifier_state == Modifier::None) {
      switch (keyboard.key) {
      case desktop::Key::Tab: {
         m_context.toggle_active_widget();
         break;
      }
      default:
         break;
      }
   }

   if (m_modifier_state == Modifier::Control) {
      switch (keyboard.key) {
      case desktop::Key::A: {
         const ui_core::Event select_all_event = event.sub_event(ui_core::Event::Type::SelectAll);
         m_content->on_event(select_all_event);
         break;
      }
      case desktop::Key::Z: {
         const ui_core::Event undo_event = event.sub_event(ui_core::Event::Type::Undo);
         m_content->on_event(undo_event);
         break;
      }
      case desktop::Key::Y: {
         const ui_core::Event undo_event = event.sub_event(ui_core::Event::Type::Redo);
         m_content->on_event(undo_event);
         break;
      }
      default:
         break;
      }
   }
}

void SecondaryEventGenerator::on_key_released(const ui_core::Event& /*event*/, const ui_core::Event::Keyboard& keyboard)
{
   if (const auto mod = key_to_modifier(keyboard.key); mod.has_value()) {
      m_modifier_state.remove_flag(*mod);
   }
}

}// namespace triglav::desktop_ui
