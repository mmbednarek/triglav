#include "TabView.hpp"

#include <utility>

#include "DesktopUI.hpp"

#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/Viewport.hpp"

namespace triglav::desktop_ui {

constexpr auto g_itemPadding = 10.0f;

TabView::TabView(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
    ui_core::LayoutWidget(ctx, parent),
    m_state(std::move(state))
{
}

Vector2 TabView::desired_size(const Vector2 parentSize) const
{
   return this->get_measure(parentSize).size;
}

void TabView::add_to_viewport(const Vector4 dimensions, const Vector4 croppingMask)
{
   const auto& measure = this->get_measure({dimensions.z, dimensions.w});

   m_dimensions = dimensions;
   m_croppingMask = croppingMask;

   m_offsetToItem.clear();

   const bool first_init = m_labels.empty();

   const Vector4 background_dims{dimensions.x, dimensions.y, dimensions.z, measure.tab_height};
   if (m_backgroundId == 0) {
      m_backgroundId = m_context.viewport().add_rectangle({
         .rect = background_dims,
         .color = TG_THEME_VAL(background_color_darker),
         .borderRadius = {0, 0, 0, 0},
         .borderColor = {0, 0, 0, 0},
         .crop = croppingMask,
         .borderWidth = 0,
      });
   } else {
      m_context.viewport().set_rectangle_dims(m_backgroundId, background_dims, m_croppingMask);
   }

   float offset_x = dimensions.x;
   u32 index = 0;
   for (const auto& tabName : m_state.tabNames) {
      if (index == m_state.activeTab) {
         const Vector4 active_tab_dims{offset_x, dimensions.y + 0.25f * g_itemPadding, measure.tab_widths[index],
                                       measure.tab_height - 0.25f * g_itemPadding};
         if (m_activeRectId == 0) {
            m_activeRectId = m_context.viewport().add_rectangle({
               .rect = active_tab_dims,
               .color = TG_THEME_VAL(background_color_brighter),
               .borderRadius = {0.0f, 0.0f, 0.0f, 0.0f},
               .borderColor = palette::NO_COLOR,
               .crop = croppingMask,
               .borderWidth = 0.0f,
            });
         } else {
            m_context.viewport().set_rectangle_dims(m_activeRectId, active_tab_dims, m_croppingMask);
         }

         const Vector4 highlight_tab_dims{offset_x, dimensions.y, measure.tab_widths[index], 0.25f * g_itemPadding};
         if (m_highlightRectId == 0) {
            m_highlightRectId = m_context.viewport().add_rectangle({
               .rect = highlight_tab_dims,
               .color = TG_THEME_VAL(accent_color),
               .borderRadius = {0.0f, 0.0f, 0.0f, 0.0f},
               .borderColor = palette::NO_COLOR,
               .crop = croppingMask,
               .borderWidth = 0.0f,
            });
         } else {
            m_context.viewport().set_rectangle_dims(m_highlightRectId, highlight_tab_dims, m_croppingMask);
         }
      }

      Vector2 text_pos{offset_x + g_itemPadding, dimensions.y + measure.tab_height - g_itemPadding};
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

      offset_x += measure.tab_widths[index];
      ++index;
   }

   m_children[m_state.activeTab]->add_to_viewport(
      {dimensions.x, dimensions.y + measure.tab_height, dimensions.z, dimensions.w - measure.tab_height}, croppingMask);
}

void TabView::remove_from_viewport()
{
   for (const auto label : m_labels) {
      m_context.viewport().remove_text(label);
   }
   m_labels.clear();

   m_context.viewport().remove_rectangle(m_backgroundId);
   m_backgroundId = 0;
   m_context.viewport().remove_rectangle_safe(m_activeRectId);
   m_context.viewport().remove_rectangle_safe(m_hoverRectId);
   m_context.viewport().remove_rectangle_safe(m_highlightRectId);

   m_children[m_state.activeTab]->remove_from_viewport();
}

void TabView::on_event(const ui_core::Event& event)
{
   if (this->visit_event(event)) {
      ui_core::Event subEvent(event);
      subEvent.mousePosition.y -= this->get_measure({m_dimensions.z, m_dimensions.w}).tab_height;
      m_children[m_state.activeTab]->on_event(subEvent);
   }
}
bool TabView::on_mouse_moved(const ui_core::Event& event)
{
   if (event.mousePosition.y > this->get_measure({m_dimensions.z, m_dimensions.w}).tab_height) {
      m_hoveredItem = ~0u;
      if (m_hoverRectId != 0) {
         m_context.viewport().remove_rectangle(m_hoverRectId);
         m_hoverRectId = 0;
      }
      return true;
   }

   auto [offset_x, index] = this->index_from_mouse_position(event.mousePosition);
   if (index == m_hoveredItem)
      return false;

   if (m_isDragging) {
      std::swap(m_children[index], m_children[m_state.activeTab]);
      std::swap(m_state.tabNames[index], m_state.tabNames[m_state.activeTab]);
      m_state.activeTab = index;

      m_cachedMeasure.reset();
      this->remove_from_viewport();
      this->add_to_viewport(m_dimensions, m_croppingMask);
      return false;
   }

   m_hoveredItem = index;

   if (index == ~0u || index == m_state.activeTab) {
      if (m_hoverRectId != 0) {
         m_context.viewport().remove_rectangle(m_hoverRectId);
         m_hoverRectId = 0;
      }
      return false;
   }

   const auto& measure = this->get_measure({m_dimensions.z, m_dimensions.w});
   const Vector4 hover_tab_dims{m_dimensions.x + offset_x, m_dimensions.y, measure.tab_widths[index], measure.tab_height};
   if (m_hoverRectId == 0) {
      m_hoverRectId = m_context.viewport().add_rectangle({
         .rect = hover_tab_dims,
         .color = TG_THEME_VAL(background_color_brighter),
         .borderRadius = {0, 0, 0, 0},
         .borderColor = palette::NO_COLOR,
         .crop = m_croppingMask,
         .borderWidth = 0.0f,
      });
   } else {
      m_context.viewport().set_rectangle_dims(m_hoverRectId, hover_tab_dims, m_croppingMask);
   }

   return false;
}

bool TabView::on_mouse_pressed(const ui_core::Event& event, const ui_core::Event::Mouse& /*mouse*/)
{
   if (event.mousePosition.y > this->get_measure({m_dimensions.z, m_dimensions.w}).tab_height) {
      return true;
   }

   auto [offset_x, index] = this->index_from_mouse_position(event.mousePosition);
   if (index == ~0u) {
      return false;
   }

   if (m_hoverRectId != 0) {
      m_context.viewport().remove_rectangle(m_hoverRectId);
      m_hoverRectId = 0;
   }

   this->set_active_tab(index);
   m_isDragging = true;
   return false;
}

bool TabView::on_mouse_released(const ui_core::Event&, const ui_core::Event::Mouse&)
{
   m_isDragging = false;
   return true;
}

void TabView::set_active_tab(const u32 activeTab)
{
   m_children[m_state.activeTab]->remove_from_viewport();
   m_state.activeTab = activeTab;
   this->add_to_viewport(m_dimensions, m_croppingMask);
}

[[nodiscard]] std::pair<float, u32> TabView::index_from_mouse_position(const Vector2 position) const
{
   const auto it_lb = m_offsetToItem.lower_bound(position.x);
   if (it_lb == m_offsetToItem.begin()) {
      return {0.0f, ~0};
   }

   const auto it = std::prev(it_lb);
   if (position.x > (it->first + this->get_measure({m_dimensions.x, m_dimensions.y}).tab_widths[it->second])) {
      return {0.0f, ~0};
   }

   return *it;
}

[[nodiscard]] const TabView::Measure& TabView::get_measure(const Vector2 availableSize) const
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
      const auto name_measure = atlas.measure_text(name.view());
      const auto width = 2 * g_itemPadding + name_measure.width;
      item_widths.emplace_back(width);
      max_height = std::max(max_height, name_measure.height);
      total_tab_width += width;
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

   m_cachedMeasure.emplace(Measure{
      .size = {total_tab_width + max_widget_width, total_tab_height + max_widget_height},
      .tab_widths = std::move(item_widths),
      .tab_height = 2 * g_itemPadding + max_height,
   });
   m_cachedMeasureSize = availableSize;
   return *m_cachedMeasure;
}

}// namespace triglav::desktop_ui
