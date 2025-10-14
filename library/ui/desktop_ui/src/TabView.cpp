#include "TabView.hpp"

#include "DesktopUI.hpp"

#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/Viewport.hpp"

namespace triglav::desktop_ui {

constexpr auto g_itemPadding = 8.0f;

TabView::TabView(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
    ui_core::LayoutWidget(ctx, parent),
    m_state(state)
{
}

Vector2 TabView::desired_size(const Vector2 parentSize) const
{
   return this->get_measure(parentSize).size;
}

void TabView::add_to_viewport(Vector4 dimensions, Vector4 croppingMask)
{
   auto measure = this->get_measure({dimensions.z, dimensions.w});

   m_dimensions = dimensions;
   m_croppingMask = croppingMask;

   m_offsetToItem.clear();

   const bool first_init = m_labels.empty();


   float offset_x = dimensions.x;
   u32 index = 0;
   for (const auto& tabName : m_state.tabNames) {
      Vector2 text_pos{offset_x + g_itemPadding, dimensions.y + g_itemPadding + measure.item_height};
      if (first_init) {
         m_labels.emplace_back(m_context.viewport().add_text({
            .content = tabName,
            .typefaceName = TG_THEME_VAL(base_typeface),
            .fontSize = TG_THEME_VAL(base_font_size),
            .position = text_pos,
            .color = TG_THEME_VAL(foreground_color),
            .crop = croppingMask,
         }));
      } else {
         m_context.viewport().set_text_position(m_labels[index], text_pos, croppingMask);
      }

      m_offsetToItem[offset_x - dimensions.x] = index;

      offset_x += 2 * g_itemPadding + measure.item_widths[index];
      ++index;
   }

   m_children[m_state.activeTab]->add_to_viewport({dimensions.x, dimensions.y + 2 * g_itemPadding + measure.item_height, dimensions.z,
                                                   dimensions.w - 2 * g_itemPadding - measure.item_height},
                                                  croppingMask);
}

void TabView::remove_from_viewport()
{
   for (const auto label : m_labels) {
      m_context.viewport().remove_text(label);
   }
   m_children[m_state.activeTab]->remove_from_viewport();
}

void TabView::on_event(const ui_core::Event& event)
{
   if (this->visit_event(event)) {
      ui_core::Event subEvent(event);
      subEvent.mousePosition.y -= 2 * g_itemPadding + this->get_measure({m_dimensions.z, m_dimensions.w}).item_height;
      m_children[m_state.activeTab]->on_event(subEvent);
   }
}

bool TabView::on_mouse_pressed(const ui_core::Event& event, const ui_core::Event::Mouse& /*mouse*/)
{
   if (event.mousePosition.y > 2 * g_itemPadding + this->get_measure({m_dimensions.z, m_dimensions.w}).item_height) {
      return true;
   }

   auto [offset_x, index] = this->index_from_mouse_position(event.mousePosition);
   if (index == ~0) {
      return false;
   }

   this->set_active_tab(index);
   return false;
}

void TabView::set_active_tab(const u32 activeTab)
{
   auto measure = this->get_measure({m_dimensions.z, m_dimensions.w});
   m_children[m_state.activeTab]->remove_from_viewport();
   m_state.activeTab = activeTab;
   m_children[m_state.activeTab]->add_to_viewport(
      {m_dimensions.x, m_dimensions.y + measure.item_height, m_dimensions.z, m_dimensions.w - measure.item_height}, m_croppingMask);
}

[[nodiscard]] std::pair<float, u32> TabView::index_from_mouse_position(Vector2 position) const
{
   const auto it_lb = m_offsetToItem.lower_bound(position.x);
   if (it_lb == m_offsetToItem.begin()) {
      return {0.0f, ~0};
   }

   const auto it = std::prev(it_lb);
   if (position.x > (it->first + 2 * g_itemPadding + this->get_measure({m_dimensions.x, m_dimensions.y}).item_widths[it->second])) {
      return {0.0f, ~0};
   }

   return *it;
}

[[nodiscard]] TabView::Measure TabView::get_measure(const Vector2 availableSize) const
{
   if (m_cachedMeasure.has_value() && m_cachedMeasureSize == availableSize)
      return *m_cachedMeasure;

   float max_height = 0.0f;
   float total_tab_width = 0.0f;
   std::vector<float> item_widths;

   const auto& atlas = m_context.glyph_cache().find_glyph_atlas({
      .typeface = TG_THEME_VAL(base_typeface),
      .fontSize = TG_THEME_VAL(base_font_size),
   });

   for (const auto& name : m_state.tabNames) {
      auto name_measure = atlas.measure_text(name.view());
      item_widths.emplace_back(name_measure.width);
      max_height = std::max(max_height, name_measure.height);
      total_tab_width += 2 * g_itemPadding + name_measure.width;
   }

   const auto total_tab_height = 2 * g_itemPadding + max_height;
   const auto widget_area = availableSize - Vector2{total_tab_width, total_tab_height};

   float max_widget_width = 0.0f;
   float max_widget_height = 0.0f;

   for (const auto& widget : m_children) {
      const auto size = widget->desired_size(widget_area);
      max_widget_width = std::max(max_widget_width, size.x);
      max_widget_height = std::max(max_widget_height, size.y);
   }

   Measure measure{
      .size = {total_tab_width + max_widget_width, total_tab_height + max_widget_height},
      .item_widths = std::move(item_widths),
      .item_height = max_height,
   };
   m_cachedMeasure.emplace(measure);
   m_cachedMeasureSize = availableSize;
   return measure;
}

}// namespace triglav::desktop_ui
