#include "Button.hpp"

#include "triglav/ui_core/UICore.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"
#include "triglav/ui_core/widget/TextBox.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

namespace triglav::desktop_ui {

Button::Button(ui_core::Context& ctx, const Button::State state, ui_core::IWidget* /*parent*/) :
    m_state(state),
    m_button(ctx, {}, this)
{
   const auto& props = m_state.manager->properties();
   m_rect = &m_button.create_content<ui_core::RectBox>({
      .color = props.button_bg_color,
   });
   auto& layout = m_rect->create_content<ui_core::VerticalLayout>({
      .padding{10.0f, 10.0f, 10.0f, 10.0f},
      .separation = 0.0f,
   });
   m_label = &layout.create_child<ui_core::TextBox>({
      .fontSize = props.button_font_size,
      .typeface = props.base_typeface,
      .content = state.label,
      .color = props.foreground_color,
      .horizontalAlignment = ui_core::HorizontalAlignment::Center,
      .verticalAlignment = ui_core::VerticalAlignment::Center,
   });
}

Vector2 Button::desired_size(const Vector2 parentSize) const
{
   return m_button.desired_size(parentSize);
}

void Button::add_to_viewport(const Vector4 dimensions)
{
   m_button.add_to_viewport(dimensions);
}

void Button::remove_from_viewport()
{
   m_button.remove_from_viewport();
}

void Button::on_event(const ui_core::Event& event)
{
   switch (event.eventType) {
   case ui_core::Event::Type::MousePressed:
      m_rect->set_color(m_state.manager->properties().button_bg_pressed_color);
      break;
   case ui_core::Event::Type::MouseReleased:
      m_rect->set_color(m_state.manager->properties().button_bg_hover_color);
      event_OnClick.publish(std::get<ui_core::Event::Mouse>(event.data).button);
      break;
   case ui_core::Event::Type::MouseEntered:
      m_rect->set_color(m_state.manager->properties().button_bg_hover_color);
      break;
   case ui_core::Event::Type::MouseLeft:
      m_rect->set_color(m_state.manager->properties().button_bg_color);
      break;
   default:
      break;
   }
}

}// namespace triglav::desktop_ui
