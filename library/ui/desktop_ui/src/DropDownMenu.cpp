#include "DropDownMenu.hpp"

#include "PopupManager.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/ui_core/widget/Button.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

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
      .color = m_state.menu->m_state.manager->properties().dropdown.bg,
      .border_radius = {0, 0, 0, 0},
      .border_color = {0, 0, 0, 0},
      .border_width = 0.0f,
   });
   auto& layout = m_rect->create_content<ui_core::VerticalLayout>({
      .padding = {0.0f, 6.0f, 0.0f, 6.0f},
      .separation = 0.0f,
   });
   layout.create_child<ui_core::TextBox>({
      .font_size = 14,
      .typeface = TG_THEME_VAL(base_typeface),
      .content = m_state.label,
      .color = {1, 1, 1, 1},
      .horizontal_alignment = ui_core::HorizontalAlignment::Center,
      .vertical_alignment = ui_core::VerticalAlignment::Top,
   });
}

Vector2 DropDownSelectorButton::desired_size(const Vector2 available_size) const
{
   return m_button.desired_size(available_size);
}

void DropDownSelectorButton::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   return m_button.add_to_viewport(dimensions, cropping_mask);
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
   m_rect->set_color(m_state.menu->m_state.manager->properties().dropdown.bg_hover);
}

void DropDownSelectorButton::on_leave() const
{
   m_rect->set_color(m_state.menu->m_state.manager->properties().dropdown.bg);
}

// -- DropDownSelector --

DropDownSelector::DropDownSelector(ui_core::Context& ctx, const State state, ui_core::IWidget* parent) :
    m_state(state),
    m_parent(parent),
    m_vertical_layout(ctx,
                      {
                         .padding = {0, 0, 0, 0},
                         .separation = 0.0f,
                      },
                      this)
{
   assert(m_state.menu != nullptr);
   for (const auto& [index, item] : Enumerate(m_state.menu->m_state.items)) {
      m_vertical_layout.create_child<DropDownSelectorButton>({
         .manager = m_state.menu->m_state.manager,
         .menu = m_state.menu,
         .label = item,
         .index = static_cast<u32>(index),
      });
   }
}

Vector2 DropDownSelector::desired_size(const Vector2 available_size) const
{
   return m_vertical_layout.desired_size(available_size);
}

void DropDownSelector::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   return m_vertical_layout.add_to_viewport(dimensions, cropping_mask);
}

void DropDownSelector::remove_from_viewport()
{
   return m_vertical_layout.remove_from_viewport();
}

void DropDownSelector::on_event(const ui_core::Event& event)
{
   m_vertical_layout.on_event(event);
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
              .color = m_state.manager->properties().dropdown.bg,
              .border_radius = {10.0f, 10.0f, 10.0f, 10.0f},
              .border_color = m_state.manager->properties().dropdown.border,
              .border_width = m_state.manager->properties().dropdown.border_width,
           },
           this)
{
   const auto& props = m_state.manager->properties();
   auto& layout = m_rect.create_content<ui_core::VerticalLayout>({
      .padding{8.0f, 8.0f, 8.0f, 8.0f},
      .separation = 0.0f,
   });
   m_label = &layout.create_child<ui_core::TextBox>({
      .font_size = 14,
      .typeface = props.base_typeface,
      .content = m_state.items[m_state.selected_item],
      .color = props.foreground_color,
      .horizontal_alignment = ui_core::HorizontalAlignment::Left,
      .vertical_alignment = ui_core::VerticalAlignment::Center,
   });
}

Vector2 DropDownMenu::desired_size(const Vector2 /*available_size*/) const
{
   Vector2 max{0, 0};
   const auto& atlas = m_context.glyph_cache().find_glyph_atlas({TG_THEME_VAL(base_typeface), 14});
   for (const auto& val : m_state.items) {
      auto measure = atlas.measure_text(val.view());
      max.x = std::max(max.x, measure.width);
      max.y = std::max(max.y, measure.height);
   }

   return {24.0f + max.x + max.y, 16.0f + max.y};
}

void DropDownMenu::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   m_dimensions = dimensions;
   m_cropping_mask = cropping_mask;
   m_rect.add_to_viewport(dimensions, cropping_mask);

   const Vector2 sprite_size{dimensions.w * 0.4, dimensions.w * 0.4};
   const Vector2 sprite_pos{dimensions.x + dimensions.z - 2 * sprite_size.x, dimensions.y + dimensions.w * 0.5 - sprite_size.x * 0.5};
   if (m_down_arrow == 0) {
      m_down_arrow = m_context.viewport().add_sprite({
         .texture = "texture/ui_atlas.tex"_rc,
         .position = sprite_pos,
         .size = sprite_size,
         .crop = {cropping_mask},
         .texture_region = Vector4{0, 0, 64, 64},
      });
   } else {
      m_context.viewport().set_sprite_position(m_down_arrow, sprite_pos, cropping_mask);
   }
}

void DropDownMenu::remove_from_viewport()
{
   m_rect.remove_from_viewport();
   m_context.viewport().remove_sprite(m_down_arrow);
   m_down_arrow = 0;
}

void DropDownMenu::on_event(const ui_core::Event& event)
{
   ui_core::visit_event<void>(*this, event);
}

void DropDownMenu::on_child_state_changed(IWidget& /*widget*/)
{
   m_rect.add_to_viewport(m_dimensions, m_cropping_mask);
}

u32 DropDownMenu::selected_item() const
{
   return m_state.selected_item;
}

void DropDownMenu::set_selected_item(const u32 index)
{
   m_state.selected_item = index;
   const auto& label = m_state.items.at(index);
   m_label->set_content(label.view());
   m_context.viewport().set_sprite_texture_region(m_down_arrow, {0, 0, 64, 64});

   if (m_current_popup != nullptr) {
      m_state.manager->popup_manager().close_popup(m_current_popup);
      m_current_popup = nullptr;
   }

   event_OnSelected.publish(index);
}

void DropDownMenu::on_mouse_released(const ui_core::Event& /*event*/, const ui_core::Event::Mouse& /*mouse*/)
{
   if (m_current_popup != nullptr) {
      m_state.manager->popup_manager().close_popup(m_current_popup);
      m_current_popup = nullptr;
      m_context.viewport().set_sprite_texture_region(m_down_arrow, {0, 0, 64, 64});
      return;
   }

   // Fake selector with invalid context
   const auto selector = std::make_unique<DropDownSelector>(m_context, DropDownSelector::State{.menu = this}, nullptr);
   const auto desired_dims = selector->desired_size({m_dimensions.x, 1024});

   // TODO: Add ability to update context.
   auto& popup = m_state.manager->popup_manager().create_popup_dialog({m_dimensions.x, m_dimensions.y + m_dimensions.w},
                                                                      {m_dimensions.z, desired_dims.y});
   popup.create_root_widget<DropDownSelector>({this});
   popup.initialize();
   m_current_popup = &popup;

   m_context.viewport().set_sprite_texture_region(m_down_arrow, {64, 0, 64, 64});
}

void DropDownMenu::on_mouse_entered(const ui_core::Event& /*event*/)
{
   m_rect.set_color(m_state.manager->properties().dropdown.bg_hover);
}

void DropDownMenu::on_mouse_left(const ui_core::Event& /*event*/)
{
   m_rect.set_color(m_state.manager->properties().dropdown.bg);
}

}// namespace triglav::desktop_ui
