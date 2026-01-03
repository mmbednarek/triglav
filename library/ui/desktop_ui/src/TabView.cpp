#include "TabView.hpp"

#include <utility>

#include "DesktopUI.hpp"

#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/ui_core/Context.hpp"

namespace triglav::desktop_ui {

using namespace name_literals;

constexpr auto g_item_padding = 13.0f;
constexpr auto g_icon_size = 18.0f;
constexpr auto g_separation = 8.0f;
constexpr auto g_close_btn_size = 12.0f;

BasicTabWidget::BasicTabWidget(const String& name, const ui_core::TextureRegion& icon, std::unique_ptr<ui_core::IWidget> widget) :
    m_name(name),
    m_icon(icon),
    m_widget(std::move(widget))
{
}

StringView BasicTabWidget::name() const
{
   return m_name.view();
}

ui_core::IWidget& BasicTabWidget::widget()
{
   return *m_widget;
}

const ui_core::TextureRegion& BasicTabWidget::icon() const
{
   return m_icon;
}

TabView::TabView(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
    ui_core::BaseWidget(parent),
    m_context(ctx),
    m_state(std::move(state)),
    m_background{
       .color = TG_THEME_VAL(background_color_darker),
       .border_radius = {0, 0, 0, 0},
       .border_color = {0, 0, 0, 0},
       .border_width = 0,
    },
    m_hover_rect{
       .color = TG_THEME_VAL(background_color_brighter),
       .border_radius = {5, 5, 5, 5},
       .border_color = palette::NO_COLOR,
       .border_width = 0.0f,
    },
    m_active_rect{
       .color = TG_THEME_VAL(background_color_brighter),
       .border_radius = {5.0f, 5.0f, 0.0f, 0.0f},
       .border_color = palette::NO_COLOR,
       .border_width = 0.0f,
    },
    m_close_button{
       .texture = "texture/ui_atlas.tex"_rc,
       .texture_region = Vector4{0, 64, 64, 64},
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

   if (m_tabs.empty()) {
      m_close_button.remove(m_context);
      m_active_rect.remove(m_context);
      return;
   }

   float offset_x = dimensions.x;
   u32 index = 0;
   for (const auto& tab : m_tabs) {
      auto width = measure.tab_widths[index];
      if (index == m_state.active_tab) {
         const Vector4 active_tab_dims{offset_x, dimensions.y, width, measure.tab_height};
         m_active_rect.add(m_context, active_tab_dims, cropping_mask);

         Vector4 close_btn_dims{offset_x + width - g_item_padding - g_close_btn_size / 2,
                                dimensions.y + measure.tab_height / 2 - g_close_btn_size / 2 + 1, g_close_btn_size, g_close_btn_size};
         m_close_button.add(m_context, close_btn_dims, cropping_mask);
      }

      Vector4 icon_dims{offset_x + g_item_padding, dimensions.y + measure.tab_height / 2 - g_icon_size / 2 + 1, g_icon_size, g_icon_size};
      Vector2 text_pos{offset_x + g_item_padding + g_icon_size + g_separation, dimensions.y + measure.tab_height - g_item_padding};
      if (first_init) {
         const auto& tex_region = tab->icon();
         auto& icon_instance = m_icons.emplace_back(ui_core::SpriteInstance{
            .texture = tex_region.name,
            .texture_region = tex_region.region,
         });
         icon_instance.add(m_context, icon_dims, cropping_mask);

         auto& text_instance = m_labels.emplace_back(ui_core::TextInstance{
            .content = tab->name(),
            .typeface_name = TG_THEME_VAL(base_typeface),
            .font_size = TG_THEME_VAL(tab_view.font_size),
            .color = TG_THEME_VAL(foreground_color),
         });
         text_instance.add(m_context, text_pos, cropping_mask);
      } else {
         m_icons[index].add(m_context, icon_dims, cropping_mask);
         m_labels[index].add(m_context, text_pos, cropping_mask);
      }

      m_offset_to_item[offset_x - dimensions.x] = index;

      offset_x += width;
      ++index;
   }

   m_tabs[m_state.active_tab]->widget().add_to_viewport(
      {dimensions.x, dimensions.y + measure.tab_height, dimensions.z, dimensions.w - measure.tab_height}, cropping_mask);
}

void TabView::remove_from_viewport()
{
   for (auto& label : m_labels) {
      label.remove(m_context);
   }
   m_labels.clear();
   for (auto& icon : m_icons) {
      icon.remove(m_context);
   }
   m_icons.clear();

   m_background.remove(m_context);
   m_active_rect.remove(m_context);
   m_hover_rect.remove(m_context);

   m_tabs[m_state.active_tab]->widget().remove_from_viewport();
}

void TabView::on_event(const ui_core::Event& event)
{
   if (m_tabs.empty())
      return;
   if (ui_core::visit_event<bool>(*this, event, true)) {
      ui_core::Event sub_event(event);
      sub_event.mouse_position.y -= this->get_measure({m_dimensions.z, m_dimensions.w}).tab_height;
      m_tabs[m_state.active_tab]->widget().on_event(sub_event);
   }
}

bool TabView::on_mouse_moved(const ui_core::Event& event)
{
   const auto& measure = this->get_measure({m_dimensions.z, m_dimensions.w});
   if (event.mouse_position.y > measure.tab_height) {
      m_hovered_item = ~0u;
      m_hover_rect.remove(m_context);
      return true;
   }

   auto [offset_x, index] = this->index_from_mouse_position(event.mouse_position);
   if (m_state.active_tab == index) {
      if (event.mouse_position.x > offset_x + measure.tab_widths[index] - g_icon_size - g_item_padding) {
         m_close_button.set_region(m_context, {64, 64, 64, 64});
      } else {
         m_close_button.set_region(m_context, {0, 64, 64, 64});
      }
   } else {
      m_close_button.set_region(m_context, {0, 64, 64, 64});
   }

   if (index == m_hovered_item) {
      return false;
   }

   if (m_is_dragging && index != ~0u) {
      std::swap(m_tabs[index], m_tabs[m_state.active_tab]);
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

   const Vector4 hover_tab_dims{m_dimensions.x + offset_x + 4, m_dimensions.y + 4, measure.tab_widths[index] - 8, measure.tab_height - 8};
   m_hover_rect.add(m_context, hover_tab_dims, m_cropping_mask);

   return false;
}

bool TabView::on_mouse_pressed(const ui_core::Event& event, const ui_core::Event::Mouse& /*mouse*/)
{
   const auto& measure = this->get_measure(rect_size(m_dimensions));
   if (event.mouse_position.y > measure.tab_height) {
      return true;
   }

   auto [offset_x, index] = this->index_from_mouse_position(event.mouse_position);
   if (index == ~0u) {
      return false;
   }
   if (index == m_state.active_tab) {
      if (event.mouse_position.x > offset_x + measure.tab_widths[index] - g_icon_size - g_item_padding) {
         this->remove_tab(m_state.active_tab);
      }
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
   if (active_tab >= m_tabs.size())
      return;
   if (m_state.active_tab < m_tabs.size()) {
      m_tabs[m_state.active_tab]->widget().remove_from_viewport();
   }
   m_state.active_tab = active_tab;
   event_OnChangedActiveTab.publish(m_state.active_tab, &m_tabs[m_state.active_tab]->widget());
   this->add_to_viewport(m_dimensions, m_cropping_mask);
}

void TabView::remove_tab(const u32 tab_id)
{
   if (tab_id >= m_tabs.size())
      return;

   this->remove_from_viewport();

   m_tabs.erase(m_tabs.begin() + m_state.active_tab);
   if (m_state.active_tab == tab_id && tab_id == m_tabs.size() - 1 && tab_id != 0) {
      --m_state.active_tab;
      event_OnChangedActiveTab.publish(m_state.active_tab, &m_tabs[m_state.active_tab]->widget());
   }
   if (m_tabs.empty()) {
      event_OnChangedActiveTab.publish(0, nullptr);
   }

   this->add_to_viewport(m_dimensions, m_cropping_mask);
}

ITabWidget& TabView::add_tab(ITabWidgetPtr&& widget)
{
   return *m_tabs.emplace_back(std::move(widget));
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

   for (const auto& tab : m_tabs) {
      const auto name_measure = atlas.measure_text(tab->name());
      const auto width = 2 * g_item_padding + g_icon_size + 2 * g_separation + name_measure.width + g_close_btn_size;
      item_widths.emplace_back(width);
      max_height = std::max(max_height, name_measure.height);
      total_tab_width += width;
   }

   const auto total_tab_height = 2 * g_item_padding + max_height;
   const auto widget_area = available_size - Vector2{total_tab_width, total_tab_height};

   float max_widget_width = 0.0f;
   float max_widget_height = 0.0f;

   for (const auto& tab : m_tabs) {
      const auto size = tab->widget().desired_size(widget_area);
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
