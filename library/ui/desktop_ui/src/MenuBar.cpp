#include "MenuBar.hpp"

#include "DesktopUI.hpp"
#include "MenuList.hpp"
#include "PopupManager.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/Viewport.hpp"

namespace triglav::desktop_ui {

using namespace name_literals;

constexpr auto g_item_hmargin = 10.0f;
constexpr auto g_item_vmargin = 8.0f;
constexpr auto g_global_margin = 6.0f;

MenuBar::MenuBar(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
    ui_core::BaseWidget(parent),
    m_context(ctx),
    m_state(state)
{
}

Vector2 MenuBar::desired_size(Vector2 /*available_size*/) const
{
   return this->get_measure().size;
}

void MenuBar::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   const auto measure = this->get_measure();
   m_dimensions = dimensions;
   m_cropping_mask = cropping_mask;

   if (m_background_id == 0) {
      m_background_id = m_context.viewport().add_rectangle({
         .rect = dimensions,
         .color = TG_THEME_VAL(background_color_darker),
         .border_radius = {0, 0, 0, 0},
         .border_color = palette::NO_COLOR,
         .crop = cropping_mask,
         .border_width = 0.0f,
      });
   } else {
      m_context.viewport().set_rectangle_dims(m_background_id, dimensions, cropping_mask);
   }

   m_context.viewport().remove_rectangle_safe(m_hover_rect_id);

   const auto& items = m_state.controller->children("root"_name);

   float x_offset = dimensions.x + g_global_margin;

   m_offset_to_item.clear();

   if (m_labels.empty()) {
      m_labels.reserve(items.size());
      for (const Name item_name : items) {
         m_offset_to_item[x_offset] = item_name;

         const auto& item = m_state.controller->item(item_name);
         m_labels.push_back(m_context.viewport().add_text({
            .content = item.label,
            .typeface_name = TG_THEME_VAL(base_typeface),
            .font_size = TG_THEME_VAL(base_font_size),
            .position = {x_offset + g_item_hmargin, dimensions.y + g_global_margin + g_item_vmargin + measure.item_height},
            .color = TG_THEME_VAL(foreground_color),
            .crop = cropping_mask,
         }));

         x_offset += 2 * g_item_hmargin + measure.item_widths.at(item_name);
      }
   } else {
      for (auto [index, label] : Enumerate(m_labels)) {
         m_offset_to_item[x_offset] = items[index];
         m_context.viewport().set_text_position(
            label, {x_offset + g_item_hmargin, dimensions.y + g_global_margin + g_item_vmargin + measure.item_height}, cropping_mask);
         x_offset += 2 * g_item_hmargin + measure.item_widths.at(items[index]);
      }
   }
}

void MenuBar::remove_from_viewport()
{
   m_context.viewport().remove_rectangle_safe(m_background_id);

   for (const auto label : m_labels) {
      m_context.viewport().remove_text(label);
   }
   m_labels.clear();

   m_context.viewport().remove_rectangle_safe(m_hover_rect_id);
}

void MenuBar::on_event(const ui_core::Event& event)
{
   ui_core::visit_event<void>(*this, event);
}

void MenuBar::on_mouse_moved(const ui_core::Event& event)
{
   const auto [offset_x, new_hovered_item] = this->index_from_mouse_position(event.mouse_position);
   if (new_hovered_item == m_hovered_item)
      return;

   m_hovered_item = new_hovered_item;
   m_hovered_item_offset = offset_x;

   this->close_submenu();
}

void MenuBar::on_mouse_pressed(const ui_core::Event& /*event*/, const ui_core::Event::Mouse& /*mouse*/)
{
   this->close_submenu();
   if (m_hovered_item == 0) {
      return;
   }

   const auto measure = this->get_measure();

   const Vector4 dims{m_hovered_item_offset, g_global_margin, 2 * g_item_hmargin + measure.item_widths.at(m_hovered_item),
                      2 * g_item_vmargin + measure.item_height};
   if (m_hover_rect_id != 0) {
      m_context.viewport().set_rectangle_dims(m_hover_rect_id, dims, m_cropping_mask);
   } else {
      m_hover_rect_id = m_context.viewport().add_rectangle({
         .rect = dims,
         .color = TG_THEME_VAL(active_color),
         .border_radius = {4, 4, 4, 4},
         .border_color = palette::NO_COLOR,
         .crop = m_cropping_mask,
         .border_width = 0.0f,
      });
   }

   const Vector2 offset{m_hovered_item_offset, m_dimensions.y + m_dimensions.w};
   MenuList::State child_state{
      .manager = m_state.manager,
      .controller = m_state.controller,
      .list_name = m_hovered_item,
      .screen_offset = offset,
   };
   const auto temporary_menu = std::make_unique<MenuList>(m_context, child_state, nullptr);
   const auto size = temporary_menu->desired_size({});

   auto& popup = m_state.manager->popup_manager().create_popup_dialog(offset, size);
   popup.create_root_widget<MenuList>(MenuList::State{child_state});
   popup.initialize();
   m_sub_menu = &popup;
}

void MenuBar::on_mouse_left(const ui_core::Event& /*event*/)
{
   if (m_sub_menu != nullptr) {
      m_state.manager->popup_manager().close_popup(m_sub_menu);
      m_sub_menu = nullptr;
   }

   if (m_hover_rect_id != 0) {
      m_context.viewport().remove_rectangle(m_hover_rect_id);
      m_hover_rect_id = 0;
   }
}

std::pair<float, Name> MenuBar::index_from_mouse_position(const Vector2 position) const
{
   const auto it_lb = m_offset_to_item.lower_bound(position.x);
   if (it_lb == m_offset_to_item.begin()) {
      return {0.0f, 0};
   }

   const auto it = std::prev(it_lb);
   if (position.x > (it->first + 2 * g_item_hmargin + this->get_measure().item_widths.at(it->second))) {
      return {0.0f, 0};
   }

   return *it;
}

void MenuBar::close_submenu()
{
   if (m_hover_rect_id != 0) {
      m_context.viewport().remove_rectangle(m_hover_rect_id);
      m_hover_rect_id = 0;
   }

   if (m_sub_menu != nullptr) {
      m_state.manager->popup_manager().close_popup(m_sub_menu);
      m_sub_menu = nullptr;
   }
}

MenuBar::Measure MenuBar::get_measure() const
{
   if (m_cached_measure.has_value()) {
      return *m_cached_measure;
   }

   const auto children = m_state.controller->children("root"_name);
   const auto& atlas = m_context.glyph_cache().find_glyph_atlas({
      .typeface = TG_THEME_VAL(base_typeface),
      .font_size = TG_THEME_VAL(base_font_size),
   });

   float max_height = 0.0f;
   float total_width = 0.0f;
   std::map<Name, float> item_widths;
   for (const auto child : children) {
      assert(child != "separator"_name && "Separators are not support for top level menu bar");

      const auto& item = m_state.controller->item(child);
      assert(item.is_submenu && "All top level items must be submenus");

      auto measure_text = atlas.measure_text(item.label.view());
      item_widths.emplace(child, measure_text.width);
      max_height = std::max(max_height, measure_text.height);
      total_width += measure_text.width + 2 * g_item_hmargin;
   }

   m_cached_measure = Measure{
      .size = {2 * g_global_margin + total_width, 2 * g_global_margin + 2 * g_item_vmargin + max_height},
      .item_widths = std::move(item_widths),
      .item_height = max_height,
   };
   return *m_cached_measure;
}

}// namespace triglav::desktop_ui