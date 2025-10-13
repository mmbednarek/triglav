#include "CheckBox.hpp"

#include "DesktopUI.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/Viewport.hpp"

namespace triglav::desktop_ui {

void RadioGroup::add_check_box(CheckBox* cb)
{
   m_checkBoxes.push_back(cb);
}

void RadioGroup::set_active(const CheckBox* activeCb) const
{
   for (CheckBox* checkBox : m_checkBoxes) {
      checkBox->set_state(checkBox == activeCb);
   }
}

CheckBox::CheckBox(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
    ContainerWidget(ctx, parent),
    m_state(state)
{
}

Vector2 CheckBox::desired_size(const Vector2 parentSize) const
{
   const Vector2 padding{2.0f * TG_THEME_VAL(checkbox.padding)};
   const auto child_size = m_content->desired_size(parentSize - padding);
   return child_size + padding;
}

void CheckBox::add_to_viewport(const Vector4 dimensions, const Vector4 croppingMask)
{
   auto size = this->desired_size(dimensions);

   if (m_background == 0) {
      m_background = m_context.viewport().add_rectangle({
         .rect = {dimensions.x, dimensions.y, size},
         .color = TG_THEME_VAL(background_color),
         .borderRadius = {4.0f, 4.0f, 4.0f, 4.0f},
         .borderColor = palette::NO_COLOR,
         .crop = croppingMask,
         .borderWidth = 0.0f,
      });
   } else {
      m_context.viewport().set_rectangle_dims(m_background, {dimensions.x, dimensions.y, size}, croppingMask);
   }

   const auto padding = TG_THEME_VAL(checkbox.padding);
   const Vector4 content_dims{dimensions.x + padding, dimensions.y + padding, size.x - 2 * padding, size.y - 2 * padding};
   m_content->add_to_viewport(content_dims, croppingMask);
}

void CheckBox::remove_from_viewport()
{
   m_context.viewport().remove_rectangle(m_background);
   m_content->remove_from_viewport();
}

void CheckBox::on_event(const ui_core::Event& event)
{
   if (this->visit_event(event)) {
      m_content->on_event(event);
   }
}

bool CheckBox::on_mouse_released(const ui_core::Event& /*event*/, const ui_core::Event::Mouse& /*mouse*/)
{
   if (m_state.radioGroup != nullptr) {
      m_state.radioGroup->set_active(this);
   } else {
      this->set_state(!m_state.isEnabled);
   }
   return false;
}

bool CheckBox::on_mouse_entered(const ui_core::Event& /*event*/)
{
   if (!m_state.isEnabled) {
      m_context.viewport().set_rectangle_color(m_background, TG_THEME_VAL(active_color));
   }
   return true;
}

bool CheckBox::on_mouse_left(const ui_core::Event& /*event*/)
{
   if (!m_state.isEnabled) {
      m_context.viewport().set_rectangle_color(m_background, TG_THEME_VAL(background_color));
   }
   return true;
}

void CheckBox::set_state(const bool isEnabled)
{
   m_state.isEnabled = isEnabled;
   if (m_state.isEnabled) {
      m_context.viewport().set_rectangle_color(m_background, TG_THEME_VAL(accent_color));
   } else {
      m_context.viewport().set_rectangle_color(m_background, TG_THEME_VAL(background_color));
   }
}

}// namespace triglav::desktop_ui
