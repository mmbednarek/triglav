#include "Button.hpp"

#include "triglav/desktop/ISurface.hpp"
#include "triglav/ui_core/UICore.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"
#include "triglav/ui_core/widget/TextBox.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

namespace triglav::desktop_ui {

constexpr auto g_padding = 5.0f;

Button::Button(ui_core::Context& ctx, const Button::State state, ui_core::IWidget* parent) :
    ui_core::ContainerWidget(ctx, parent),
    m_state(state),
    m_background{
       .color = TG_THEME_VAL(background_color_brighter),
       .border_radius = {5.0f, 5.0f, 5.0f, 5.0f},
       .border_color = TG_THEME_VAL(active_color),
       .border_width = 1.0f,
    }
{
}

Vector2 Button::desired_size(const Vector2 parent_size) const
{
   const auto child_size = m_content->desired_size(parent_size);
   return child_size + 2.0f * Vector2{g_padding, g_padding};
}

void Button::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   m_background.add(m_context, dimensions, cropping_mask);
   m_content->add_to_viewport(
      {dimensions.x + g_padding, dimensions.y + g_padding, dimensions.z - 2 * g_padding, dimensions.w - 2 * g_padding}, cropping_mask);
}

void Button::remove_from_viewport()
{
   m_background.remove(m_context);
   m_content->remove_from_viewport();
}

void Button::on_event(const ui_core::Event& event)
{
   switch (event.event_type) {
   case ui_core::Event::Type::MousePressed:
      m_background.set_color(m_context, m_state.manager->properties().background_color_darker);
      break;
   case ui_core::Event::Type::MouseReleased:
      m_background.set_color(m_context, m_state.manager->properties().active_color);
      event_OnClick.publish();
      break;
   case ui_core::Event::Type::MouseEntered:
      m_state.manager->surface().set_cursor_icon(desktop::CursorIcon::Hand);
      m_background.set_color(m_context, m_state.manager->properties().active_color);
      break;
   case ui_core::Event::Type::MouseLeft:
      m_state.manager->surface().set_cursor_icon(desktop::CursorIcon::Arrow);
      m_background.set_color(m_context, m_state.manager->properties().background_color_brighter);
      break;
   default:
      break;
   }
}

}// namespace triglav::desktop_ui
