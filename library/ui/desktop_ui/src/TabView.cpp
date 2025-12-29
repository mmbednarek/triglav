#include "TabView.hpp"

#include <utility>

#include "DesktopUI.hpp"

#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/ui_core/Context.hpp"

namespace triglav::desktop_ui {

using namespace name_literals;

constexpr auto g_item_padding = 13.0f;

TabView::TabView(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
    ui_core::LayoutWidget(ctx, parent),
    m_state(std::move(state)),
    m_background{
       .color = TG_THEME_VAL(background_color_darker),
       .border_radius = {0, 0, 0, 0},
       .border_color = {0, 0, 0, 0},
       .border_width = 0,
    },
    m_hover_rect{
       .color = TG_THEME_VAL(background_color_brighter),
       .border_radius = {0, 0, 0, 0},
       .border_color = palette::NO_COLOR,
       .border_width = 0.0f,
    },
    m_active_rect{
       .color = TG_THEME_VAL(background_color_brighter),
       .border_radius = {5.0f, 5.0f, 0.0f, 0.0f},
       .border_color = palette::NO_COLOR,
       .border_width = 0.0f,
    }
{
}

Vector2 TabView::desired_size(const Vector2 available_size) const
{
   return this->get_measure(available_size).size;
}

void TabView::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   const auto& measure = this->get_measure({dimensions.z, dimensions.w});

   m_dimensions = dimensions;
   m_cropping_mask = cropping_mask;

   m_offset_to_item.clear();

   const bool first_init = m_labels.empty();

   const Vector4 background_dims{dimensions.x, dimensions.y, dimensions.z, measure.tab_height};
   m_background.add(m_context, background_dims, cropping_mask);

   float offset_x = dimensions.x;
   u32 index = 0;
   for (const auto& tab_name : m_state.tab_names) {
      if (index == m_state.active_tab) {
         const Vector4 active_tab_dims{offset_x, dimensions.y + 0.25f * g_item_padding, measure.tab_widths[index],
                                       measure.tab_height - 0.25f * g_item_padding};
         m_active_rect.add(m_context, active_tab_dims, cropping_mask);
      }

      Vector2 text_pos{offset_x + g_item_padding, dimensions.y + measure.tab_height - g_item_padding};
      Vector4 close_btn_dims{offset_x + measure.tab_widths[index] - g_item_padding, dimensions.y + g_item_padding, 16, 16};
      if (first_init) {
         auto& text_instance = m_labels.emplace_back(ui_core::TextInstance{
            .content = tab_name,
            .typeface_name = TG_THEME_VAL(base_typeface),
            .font_size = TG_THEME_VAL(tab_view.font_size),
            .color = TG_THEME_VAL(foreground_color),
         });
         text_instance.add(m_context, text_pos, cropping_mask);

         auto& close_btn_instance = m_close_buttons.emplace_back(ui_core::SpriteInstance{
            .texture = "texture/ui_atlas.tex"_rc,
            .size = {16, 16},
            .texture_region = Vector4{0, 64, 64, 64},
         });
         close_btn_instance.add(m_context, close_btn_dims, cropping_mask);
      } else {
         m_labels[index].add(m_context, text_pos, cropping_mask);
         m_close_buttons[index].add(m_context, close_btn_dims, cropping_mask);
      }

      m_offset_to_item[offset_x - dimensions.x] = index;

      offset_x += measure.tab_widths[index];
      ++index;
   }

   m_children[m_state.active_tab]->add_to_viewport(
      {dimensions.x, dimensions.y + measure.tab_height, dimensions.z, dimensions.w - measure.tab_height}, cropping_mask);
}

void TabView::remove_from_viewport()
{
   for (auto& label : m_labels) {
      label.remove(m_context);
   }
   m_labels.clear();

   m_background.remove(m_context);
   m_active_rect.remove(m_context);
   m_hover_rect.remove(m_context);

   m_children[m_state.active_tab]->remove_from_viewport();
}

void TabView::on_event(const ui_core::Event& event)
{
   if (ui_core::visit_event<bool>(*this, event, true)) {
      ui_core::Event sub_event(event);
      sub_event.mouse_position.y -= this->get_measure({m_dimensions.z, m_dimensions.w}).tab_height;
      m_children[m_state.active_tab]->on_event(sub_event);
   }
}

bool TabView::on_mouse_moved(const ui_core::Event& event)
{
   if (event.mouse_position.y > this->get_measure({m_dimensions.z, m_dimensions.w}).tab_height) {
      m_hovered_item = ~0u;
      m_hover_rect.remove(m_context);
      return true;
   }

   auto [offset_x, index] = this->index_from_mouse_position(event.mouse_position);
   if (index == m_hovered_item)
      return false;

   if (m_is_dragging) {
      std::swap(m_children[index], m_children[m_state.active_tab]);
      std::swap(m_state.tab_names[index], m_state.tab_names[m_state.active_tab]);
      m_state.active_tab = index;

      m_cached_measure.reset();
      this->remove_from_viewport();
      this->add_to_viewport(m_dimensions, m_cropping_mask);
      return false;
   }

   m_hovered_item = index;

   if (index == ~0u || index == m_state.active_tab) {
      m_hover_rect.remove(m_context);
      return false;
   }

   const auto& measure = this->get_measure({m_dimensions.z, m_dimensions.w});
   const Vector4 hover_tab_dims{m_dimensions.x + offset_x, m_dimensions.y, measure.tab_widths[index], measure.tab_height};
   m_hover_rect.add(m_context, hover_tab_dims, m_cropping_mask);

   return false;
}

bool TabView::on_mouse_pressed(const ui_core::Event& event, const ui_core::Event::Mouse& /*mouse*/)
{
   if (event.mouse_position.y > this->get_measure({m_dimensions.z, m_dimensions.w}).tab_height) {
      return true;
   }

   auto [offset_x, index] = this->index_from_mouse_position(event.mouse_position);
   if (index == ~0u) {
      return false;
   }

   m_hover_rect.remove(m_context);

   this->set_active_tab(index);
   m_is_dragging = true;
   return false;
}

bool TabView::on_mouse_released(const ui_core::Event&, const ui_core::Event::Mouse&)
{
   m_is_dragging = false;
   return true;
}

void TabView::set_active_tab(const u32 active_tab)
{
   m_children[m_state.active_tab]->remove_from_viewport();
   m_state.active_tab = active_tab;
   this->add_to_viewport(m_dimensions, m_cropping_mask);
}

[[nodiscard]] std::pair<float, u32> TabView::index_from_mouse_position(const Vector2 position) const
{
   const auto it_lb = m_offset_to_item.lower_bound(position.x);
   if (it_lb == m_offset_to_item.begin()) {
      return {0.0f, ~0};
   }

   const auto it = std::prev(it_lb);
   if (position.x > (it->first + this->get_measure({m_dimensions.x, m_dimensions.y}).tab_widths[it->second])) {
      return {0.0f, ~0};
   }

   return *it;
}

[[nodiscard]] const TabView::Measure& TabView::get_measure(const Vector2 available_size) const
{
   if (m_cached_measure.has_value() && m_cached_measure_size == available_size)
      return *m_cached_measure;

   float max_height = 0.0f;
   float total_tab_width = 0.0f;
   std::vector<float> item_widths;

   const auto& atlas = m_context.glyph_cache().find_glyph_atlas({
      .typeface = TG_THEME_VAL(base_typeface),
      .font_size = TG_THEME_VAL(tab_view.font_size),
   });

   for (const auto& name : m_state.tab_names) {
      const auto name_measure = atlas.measure_text(name.view());
      const auto width = 2 * g_item_padding + name_measure.width;
      item_widths.emplace_back(width);
      max_height = std::max(max_height, name_measure.height);
      total_tab_width += width;
   }

   const auto total_tab_height = 2 * g_item_padding + max_height;
   const auto widget_area = available_size - Vector2{total_tab_width, total_tab_height};

   float max_widget_width = 0.0f;
   float max_widget_height = 0.0f;

   for (const auto& widget : m_children) {
      const auto size = widget->desired_size(widget_area);
      max_widget_width = std::max(max_widget_width, size.x);
      max_widget_height = std::max(max_widget_height, size.y);
   }

   m_cached_measure.emplace(Measure{
      .size = {total_tab_width + max_widget_width, total_tab_height + max_widget_height},
      .tab_widths = std::move(item_widths),
      .tab_height = 2 * g_item_padding + max_height,
   });
   m_cached_measure_size = available_size;
   return *m_cached_measure;
}

}// namespace triglav::desktop_ui
