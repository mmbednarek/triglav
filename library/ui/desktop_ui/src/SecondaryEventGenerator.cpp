#include "SecondaryEventGenerator.hpp"

#include "triglav/ui_core/Context.hpp"

namespace triglav::desktop_ui {

using desktop::Modifier;

SecondaryEventGenerator::SecondaryEventGenerator(ui_core::Context& ctx, ui_core::IWidget* parent, desktop::ISurface& surface) :
    ui_core::ProxyWidget(ctx, parent),
    m_surface(surface)
{
}

void SecondaryEventGenerator::on_event(const ui_core::Event& event)
{
   ui_core::visit_event<void>(*this, event);
   this->forward_event(event);
}

void SecondaryEventGenerator::on_mouse_pressed(const ui_core::Event& event, const ui_core::Event::Mouse& /*mouse*/) const
{
   if (m_context.active_widget() != nullptr) {
      if (!is_point_inside(m_context.active_area(), event.global_mouse_position)) {
         m_context.set_active_widget(nullptr, {}, {});
      }
   }
}

void SecondaryEventGenerator::on_key_pressed(const ui_core::Event& event, const ui_core::Event::Keyboard& keyboard)
{
   const auto modifier_state = m_surface.modifiers();
   if (modifier_state == Modifier::Empty) {
      switch (keyboard.key) {
      case desktop::Key::Tab: {
         m_context.toggle_active_widget();
         break;
      }
      default:
         break;
      }
   }

   if (modifier_state == Modifier::Control) {
      switch (keyboard.key) {
      case desktop::Key::A: {
         const ui_core::Event select_all_event = event.sub_event(ui_core::Event::Type::SelectAll);
         this->forward_event(select_all_event);
         break;
      }
      default:
         break;
      }
   }
}

void SecondaryEventGenerator::forward_event(const ui_core::Event& event)
{
   if (m_context.active_widget() != nullptr && m_context.should_redirect_event(event.event_type)) {
      ui_core::Event sub_event(event);
      sub_event.mouse_position = event.global_mouse_position - rect_position(m_context.active_area());
      m_context.active_widget()->on_event(sub_event);
   } else {
      ProxyWidget::on_event(event);
   }
}

}// namespace triglav::desktop_ui
