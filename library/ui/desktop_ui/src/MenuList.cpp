#include "MenuList.hpp"

#include "DesktopUI.hpp"
#include "PopupManager.hpp"

#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/Viewport.hpp"

namespace triglav::desktop_ui {

using namespace name_literals;
using namespace string_literals;

constexpr auto g_item_hmargin = 10.0f;
constexpr auto g_item_vmargin = 8.0f;
constexpr auto g_global_margin = 6.0f;
constexpr auto g_indicator_size = 16.0f;
constexpr auto g_indicator_margin = 8.0f;
constexpr auto g_separator_height = 2.0f;

MenuList::MenuList(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
    ui_core::BaseWidget(parent),
    m_context(ctx),
    m_state(state)
{
}

Vector2 MenuList::desired_size(const Vector2 /*available_size*/) const
{
   return this->get_measure().size;
}

void MenuList::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   const auto measure = this->get_measure();
   m_cropping_mask = cropping_mask;

   m_background_id = m_context.viewport().add_rectangle({
      .rect = dimensions,
      .color = TG_THEME_VAL(background_color_darker),
      .border_radius = palette::NO_COLOR,
      .border_color = TG_THEME_VAL(active_color),
      .crop = cropping_mask,
      .border_width = 1.0f,
   });

   const auto& items = m_state.controller->children(m_state.list_name);

   float y_offset = dimensions.y + g_global_margin;

   if (m_labels.empty()) {
      m_labels.reserve(items.size());
      for (const Name item_name : items) {
         if (item_name == "separator"_name) {
            m_separators.push_back(m_context.viewport().add_rectangle({
               .rect = {dimensions.x + g_global_margin, y_offset + (measure.separation_height - g_separator_height) / 2,
                        measure.item_size.x, g_separator_height},
               .color = TG_THEME_VAL(active_color),
               .border_radius = {0, 0, 0, 0},
               .border_color = palette::NO_COLOR,
               .crop = cropping_mask,
               .border_width = 0.0f,
            }));
            y_offset += measure.separation_height;
            continue;
         }

         m_height_to_item[y_offset] = item_name;

         const auto& item = m_state.controller->item(item_name);
         m_labels.push_back(m_context.viewport().add_text({
            .content = item.label,
            .typeface_name = TG_THEME_VAL(base_typeface),
            .font_size = TG_THEME_VAL(base_font_size),
            .position = {dimensions.x + g_item_hmargin + g_global_margin, y_offset + measure.text_height + g_item_vmargin},
            .color = TG_THEME_VAL(foreground_color),
            .crop = cropping_mask,
         }));

         if (item.is_submenu) {
            m_icons.push_back(m_context.viewport().add_sprite({
               .texture = "texture/ui_atlas.tex"_rc,
               .position = {dimensions.z - g_global_margin - g_indicator_margin - g_indicator_size,
                            y_offset + (measure.item_size.y - g_indicator_size) / 2},
               .size = {16, 16},
               .crop = {cropping_mask},
               .texture_region = Vector4{3 * 64, 0, 64, 64},
            }));
         }

         y_offset += measure.item_size.y;
      }
   }
}

void MenuList::remove_from_viewport()
{
   if (m_sub_menu != nullptr) {
      m_state.manager->popup_manager().close_popup(m_sub_menu);
      m_sub_menu = nullptr;
   }
   for (const auto label : m_labels) {
      m_context.viewport().remove_text(label);
   }
   for (const auto icon : m_icons) {
      m_context.viewport().remove_sprite(icon);
   }
   for (const auto separator : m_separators) {
      m_context.viewport().remove_rectangle(separator);
   }
   m_context.viewport().remove_rectangle(m_background_id);
   if (m_hover_rect_id != 0) {
      m_context.viewport().remove_rectangle(m_hover_rect_id);
   }
}

void MenuList::on_event(const ui_core::Event& event)
{
   ui_core::visit_event<void>(*this, event);
}

void MenuList::on_mouse_moved(const ui_core::Event& event)
{
   const auto measure = this->get_measure();

   const auto [offset_y, new_hovered_item] = this->index_from_mouse_position(event.mouse_position);
   if (new_hovered_item == m_hovered_item)
      return;

   m_hovered_item = new_hovered_item;

   if (m_sub_menu != nullptr) {
      m_state.manager->popup_manager().close_popup(m_sub_menu);
      m_sub_menu = nullptr;
   }

   if (m_hovered_item == 0) {
      if (m_hover_rect_id != 0) {
         m_context.viewport().remove_rectangle(m_hover_rect_id);
         m_hover_rect_id = 0;
      }
      return;
   }

   const Vector4 dims{g_global_margin, offset_y, measure.item_size.x, measure.item_size.y};
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

   auto& item = m_state.controller->item(m_hovered_item);
   if (item.is_submenu) {
      const Vector2 offset = m_state.screen_offset + Vector2{measure.size.x, offset_y};

      State child_state{
         .manager = m_state.manager,
         .controller = m_state.controller,
         .list_name = m_hovered_item,
         .screen_offset = offset,
      };
      const auto temporary_menu = std::make_unique<MenuList>(m_context, child_state, nullptr);
      const auto size = temporary_menu->desired_size({});

      auto& popup = m_state.manager->popup_manager().create_popup_dialog(offset, size);
      popup.create_root_widget<MenuList>(State{child_state});
      popup.initialize();
      m_sub_menu = &popup;
   }
}

void MenuList::on_mouse_released(const ui_core::Event& /*event*/, const ui_core::Event::Mouse& /*mouse*/)
{
   if (m_hovered_item != 0) {
      m_state.controller->trigger_on_clicked(m_hovered_item);
   }
}

std::pair<float, Name> MenuList::index_from_mouse_position(const Vector2 position) const
{
   const auto it_lb = m_height_to_item.lower_bound(position.y);
   if (it_lb == m_height_to_item.begin()) {
      return {0.0f, 0};
   }

   const auto it = std::prev(it_lb);
   if (position.y > it->first + this->get_measure().item_size.y) {
      return {0.0f, 0};
   }

   return *it;
}

MenuList::Measure MenuList::get_measure() const
{
   if (m_cached_measure.has_value()) {
      return *m_cached_measure;
   }

   const auto children = m_state.controller->children(m_state.list_name);
   const auto& atlas = m_context.glyph_cache().find_glyph_atlas({
      .typeface = TG_THEME_VAL(base_typeface),
      .font_size = TG_THEME_VAL(base_font_size),
   });

   float max_height = 0.0f;
   float max_width = 0.0f;
   u32 separator_count = 0;
   for (const auto child : children) {
      if (child == "separator"_name) {
         ++separator_count;
         continue;
      }
      const auto& item = m_state.controller->item(child);
      auto measure_text = atlas.measure_text(item.label.view());
      if (item.is_submenu) {
         measure_text.width += g_indicator_margin + g_indicator_size;
      }
      max_width = std::max(max_width, measure_text.width);
      max_height = std::max(max_height, measure_text.height);
   }

   const u32 item_count = static_cast<u32>(children.size()) - separator_count;

   const Vector2 item_size{2 * g_item_hmargin + max_width, 2 * g_item_vmargin + max_height};
   const float separation_height = 0.5f * item_size.y;
   m_cached_measure = Measure{
      .size = {2 * g_global_margin + item_size.x, 2 * g_global_margin + item_size.y * static_cast<float>(item_count) +
                                                     separation_height * static_cast<float>(separator_count)},
      .item_size = item_size,
      .text_height = max_height,
      .separation_height = separation_height,
      .separator_count = separator_count,
   };
   return *m_cached_measure;
}

}// namespace triglav::desktop_ui