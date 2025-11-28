#include "CheckBox.hpp"

#include "DesktopUI.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/Viewport.hpp"

namespace triglav::desktop_ui {

void RadioGroup::add_check_box(CheckBox* cb)
{
   m_check_boxes.push_back(cb);
}

void RadioGroup::set_active(const CheckBox* active_cb) const
{
   const auto it = std::ranges::find(m_check_boxes, active_cb);
   if (it != m_check_boxes.end()) {
      event_OnSelection.publish(static_cast<u32>(it - m_check_boxes.begin()));
   }

   for (CheckBox* check_box : m_check_boxes) {
      check_box->set_state(check_box == active_cb);
   }
}

CheckBox::CheckBox(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
    ContainerWidget(ctx, parent),
    m_state(state),
    m_background{
       .color = TG_THEME_VAL(background_color_brighter),
       .border_radius = {4.0f, 4.0f, 4.0f, 4.0f},
       .border_color = palette::NO_COLOR,
       .border_width = 0.0f,
    }
{
}

Vector2 CheckBox::desired_size(const Vector2 parent_size) const
{
   const Vector2 padding{2.0f * TG_THEME_VAL(checkbox.padding)};
   const auto child_size = m_content->desired_size(parent_size - padding);
   return child_size + padding;
}

void CheckBox::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   auto size = this->desired_size({dimensions.z, dimensions.w});

   m_background.add(m_context, {dimensions.x, dimensions.y, size}, cropping_mask);

   const auto padding = TG_THEME_VAL(checkbox.padding);
   const Vector4 content_dims{dimensions.x + padding, dimensions.y + padding, size.x - 2 * padding, size.y - 2 * padding};
   m_content->add_to_viewport(content_dims, cropping_mask);
}

void CheckBox::remove_from_viewport()
{
   m_background.remove(m_context);
   m_content->remove_from_viewport();
}

void CheckBox::on_event(const ui_core::Event& event)
{
   if (ui_core::visit_event<bool>(*this, event, true)) {
      m_content->on_event(event);
   }
}

bool CheckBox::on_mouse_released(const ui_core::Event& /*event*/, const ui_core::Event::Mouse& /*mouse*/)
{
   if (m_state.radio_group != nullptr) {
      m_state.radio_group->set_active(this);
   } else {
      this->set_state(!m_state.is_enabled);
   }
   return false;
}

bool CheckBox::on_mouse_entered(const ui_core::Event& /*event*/)
{
   if (!m_state.is_enabled) {
      m_background.set_color(m_context, TG_THEME_VAL(active_color));
   }
   return true;
}

bool CheckBox::on_mouse_left(const ui_core::Event& /*event*/)
{
   if (!m_state.is_enabled) {
      m_background.set_color(m_context, TG_THEME_VAL(background_color_brighter));
   }
   return true;
}

void CheckBox::set_state(const bool is_enabled)
{
   m_state.is_enabled = is_enabled;
   if (m_state.is_enabled) {
      m_background.set_color(m_context, TG_THEME_VAL(accent_color));
   } else {
      m_background.set_color(m_context, TG_THEME_VAL(background_color_brighter));
   }
}

}// namespace triglav::desktop_ui
