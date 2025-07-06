#include "DropDownMenu.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/ui_core/widget/Button.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

#include "DialogManager.hpp"

#include <memory>
#include <utility>

namespace triglav::desktop_ui {

using namespace name_literals;

// -- DropDownSelectorButton --

DropDownSelectorButton::DropDownSelectorButton(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
    m_state(std::move(state)),
    m_parent(parent),
    m_button(ctx, {}, this),
    TG_CONNECT(m_button, OnClick, on_click),
    TG_CONNECT(m_button, OnEnter, on_enter),
    TG_CONNECT(m_button, OnLeave, on_leave)
{
   m_rect = &m_button.create_content<ui_core::RectBox>({
      .color = m_state.menu->m_state.manager->properties().dropdown_bg,
      .borderRadius = {0, 0, 0, 0},
      .borderColor = {0, 0, 0, 0},
      .borderWidth = 0.0f,
   });
   auto& layout = m_rect->create_content<ui_core::VerticalLayout>({
      .padding = {0.0f, 10.0f, 0.0f, 10.0f},
      .separation = 0.0f,
   });
   layout.create_child<ui_core::TextBox>({
      .fontSize = 16,
      .typeface = "cantarell.typeface"_rc,
      .content = m_state.label,
      .color = {1, 1, 1, 1},
      .horizontalAlignment = ui_core::HorizontalAlignment::Center,
      .verticalAlignment = ui_core::VerticalAlignment::Top,
   });
}

Vector2 DropDownSelectorButton::desired_size(const Vector2 parentSize) const
{
   return m_button.desired_size(parentSize);
}

void DropDownSelectorButton::add_to_viewport(const Vector4 dimensions)
{
   return m_button.add_to_viewport(dimensions);
}

void DropDownSelectorButton::remove_from_viewport()
{
   return m_button.remove_from_viewport();
}

void DropDownSelectorButton::on_event(const ui_core::Event& event)
{
   m_button.on_event(event);
}
void DropDownSelectorButton::on_child_state_changed(IWidget& widget)
{
   if (m_parent != nullptr) {
      m_parent->on_child_state_changed(widget);
   }
}

void DropDownSelectorButton::on_click(const desktop::MouseButton button) const
{
   if (button != desktop::MouseButton::Left)
      return;

   m_state.menu->set_selected_item(m_state.index);
}

void DropDownSelectorButton::on_enter() const
{
   m_rect->set_color(m_state.menu->m_state.manager->properties().dropdown_bg_hover);
}

void DropDownSelectorButton::on_leave() const
{
   m_rect->set_color(m_state.menu->m_state.manager->properties().dropdown_bg);
}

// -- DropDownSelector --

DropDownSelector::DropDownSelector(ui_core::Context& ctx, const State state, ui_core::IWidget* parent) :
    m_state(state),
    m_parent(parent),
    m_verticalLayout(ctx,
                     {
                        .padding = {0, 0, 0, 0},
                        .separation = 0.0f,
                     },
                     this)
{
   assert(m_state.menu != nullptr);
   for (const auto& [index, item] : Enumerate(m_state.menu->m_state.items)) {
      m_verticalLayout.create_child<DropDownSelectorButton>({
         .label = item,
         .index = index,
         .menu = m_state.menu,
      });
   }
}

Vector2 DropDownSelector::desired_size(const Vector2 parentSize) const
{
   return m_verticalLayout.desired_size(parentSize);
}

void DropDownSelector::add_to_viewport(const Vector4 dimensions)
{
   return m_verticalLayout.add_to_viewport(dimensions);
}

void DropDownSelector::remove_from_viewport()
{
   return m_verticalLayout.remove_from_viewport();
}

void DropDownSelector::on_event(const ui_core::Event& event)
{
   m_verticalLayout.on_event(event);
}

void DropDownSelector::on_child_state_changed(IWidget& widget)
{
   if (m_parent != nullptr) {
      m_parent->on_child_state_changed(widget);
   }
}

// -- DropDownMenu --

DropDownMenu::DropDownMenu(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
    m_context(ctx),
    m_state(std::move(state)),
    m_parent(parent),
    m_rect(ctx,
           {
              .color = m_state.manager->properties().dropdown_bg,
              .borderRadius = {10.0f, 10.0f, 10.0f, 10.0f},
              .borderColor = m_state.manager->properties().dropdown_border,
              .borderWidth = m_state.manager->properties().dropdown_border_width,
           },
           this)
{
   const auto& props = m_state.manager->properties();
   auto& layout = m_rect.create_content<ui_core::VerticalLayout>({
      .padding{15.0f, 15.0f, 15.0f, 15.0f},
      .separation = 0.0f,
   });
   m_label = &layout.create_child<ui_core::TextBox>({
      .fontSize = props.button_font_size,
      .typeface = props.base_typeface,
      .content = m_state.items[m_state.selectedItem],
      .color = props.foreground_color,
      .horizontalAlignment = ui_core::HorizontalAlignment::Center,
      .verticalAlignment = ui_core::VerticalAlignment::Center,
   });
}

Vector2 DropDownMenu::desired_size(const Vector2 parentSize) const
{
   return m_rect.desired_size(parentSize);
}

void DropDownMenu::add_to_viewport(const Vector4 dimensions)
{
   m_dimensions = dimensions;
   m_rect.add_to_viewport(dimensions);
}

void DropDownMenu::remove_from_viewport()
{
   m_rect.remove_from_viewport();
}

void DropDownMenu::on_event(const ui_core::Event& event)
{
   switch (event.eventType) {
   case ui_core::Event::Type::MouseReleased: {
      if (m_currentPopup != nullptr) {
         m_state.manager->dialog_manager().close_popup(m_currentPopup);
         m_currentPopup = nullptr;
      } else {
         // Fake selector with invalid context
         const auto selector = std::make_unique<DropDownSelector>(m_context, DropDownSelector::State{.menu = this}, nullptr);
         const auto desiredDims = selector->desired_size({m_dimensions.x, 1024});

         // TODO: Add ability to update context.
         auto& popup = m_state.manager->dialog_manager().create_popup_dialog({m_dimensions.x, m_dimensions.y + m_dimensions.w},
                                                                             {m_dimensions.z, desiredDims.y});
         popup.create_root_widget<DropDownSelector>({this});
         popup.initialize();
         m_currentPopup = &popup;
      }
      break;
   }
   case ui_core::Event::Type::MouseEntered:
      m_rect.set_color(m_state.manager->properties().dropdown_bg_hover);
      break;
   case ui_core::Event::Type::MouseLeft:
      m_rect.set_color(m_state.manager->properties().dropdown_bg);
      break;
   default:
      break;
   }
}

void DropDownMenu::on_child_state_changed(IWidget& /*widget*/)
{
   m_rect.add_to_viewport(m_dimensions);
}

void DropDownMenu::set_selected_item(const u32 index)
{
   m_state.selectedItem = index;
   const auto& label = m_state.items.at(index);
   m_label->set_content(label.view());

   if (m_currentPopup != nullptr) {
      m_state.manager->dialog_manager().close_popup(m_currentPopup);
      m_currentPopup = nullptr;
   }
}

}// namespace triglav::desktop_ui
