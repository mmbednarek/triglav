#include "TreeView.hpp"

#include <utility>

#include "DesktopUI.hpp"

#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/Viewport.hpp"

namespace triglav::desktop_ui {

using namespace name_literals;

constexpr auto g_indent_width = 16.0f;
constexpr auto g_global_padding = 10.0f;
constexpr auto g_item_padding = 6.0f;

TreeView::TreeView(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
    ui_core::BaseWidget(parent),
    m_context(ctx),
    m_state(std::move(state)),
    m_item_highlight{
       .color = TG_THEME_VAL(active_color),
       .border_radius = {0, 0, 0, 0},
       .border_color = palette::NO_COLOR,
       .border_width = 0.0f,
    }
{
}

Vector2 TreeView::desired_size(Vector2 /*parent_size*/) const
{
   return this->get_measure().size;
}

void TreeView::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   m_dimensions = dimensions;
   m_cropping_mask = cropping_mask;
   m_offset_to_item_id.clear();
   float offset_y = g_global_padding + dimensions.y;
   this->draw_level(TREE_ROOT, dimensions.x + g_global_padding, offset_y);


   if (m_selected_item != TREE_ROOT) {
      const auto highlight_dims = m_highlight_dims + Vector4{m_dimensions.x, m_dimensions.y, m_dimensions.z, 0};
      m_item_highlight.add(m_context, highlight_dims, m_cropping_mask);
   }
}

void TreeView::remove_from_viewport()
{
   for (auto& label : std::views::values(m_labels)) {
      label.remove(m_context);
   }
   m_labels.clear();
   for (auto& icon : std::views::values(m_icons)) {
      icon.remove(m_context);
   }
   m_icons.clear();
   for (auto& arrow : std::views::values(m_arrows)) {
      arrow.remove(m_context);
   }
   m_arrows.clear();

   m_item_highlight.remove(m_context);
}

void TreeView::on_event(const ui_core::Event& event)
{
   ui_core::visit_event<void>(*this, event);
}

void TreeView::on_mouse_pressed(const ui_core::Event& event, const ui_core::Event::Mouse& /*mouse*/)
{
   auto [base_offset, item_id] = this->index_from_mouse_position(event.mouse_position);
   m_selected_item = item_id;
   if (item_id == 0) {
      m_item_highlight.remove(m_context);
      return;
   }
   const auto& item = m_state.controller->item(item_id);

   const auto measure = this->get_measure();
   m_highlight_dims = Vector4{0, base_offset, 0, 2 * g_item_padding + measure.item_size.y};
   const auto highlight_dims = m_highlight_dims + Vector4{m_dimensions.x, m_dimensions.y, 0, 0};

   m_item_highlight.add(m_context, highlight_dims, m_cropping_mask);

   if (item.has_children) {
      if (m_state.extended_items.contains(item_id)) {
         m_state.extended_items.erase(item_id);
         this->remove_level(item_id);
      } else {
         m_state.extended_items.insert(item_id);
      }
   }

   this->add_to_viewport(m_dimensions, m_cropping_mask);
}

std::pair<float, TreeItemId> TreeView::index_from_mouse_position(const Vector2 position) const
{
   const auto it_lb = m_offset_to_item_id.lower_bound(position.y);
   if (it_lb == m_offset_to_item_id.begin()) {
      return {0.0f, 0};
   }

   const auto it = std::prev(it_lb);
   if (position.y > it->first + this->get_measure().item_size.y + 2 * g_item_padding) {
      return {0.0f, 0};
   }

   return *it;
}

TreeView::Measure TreeView::get_measure(const TreeItemId parent_id) const
{
   if (parent_id == 0 && m_cached_measure.has_value()) {
      return m_cached_measure.value();
   }

   float max_width = 0;
   float max_height = 0;
   u32 total_count = 0;

   const auto& atlas = m_context.glyph_cache().find_glyph_atlas({
      .typeface = TG_THEME_VAL(base_typeface),
      .font_size = TG_THEME_VAL(base_font_size),
   });

   const auto& children = m_state.controller->children(parent_id);
   for (const auto child : children) {
      auto item = m_state.controller->item(child);
      auto measured_label = atlas.measure_text(item.label.view());

      max_width = std::max(max_width, measured_label.width);
      max_height = std::max(max_height, measured_label.height);

      ++total_count;

      if (item.has_children && m_state.extended_items.contains(child)) {
         auto sub_measure = this->get_measure(child);
         max_width = std::max(max_width, sub_measure.item_size.x + g_indent_width);
         max_height = std::max(max_height, sub_measure.item_size.y);
         total_count += sub_measure.total_count;
      }
   }

   Measure measure{
      .size = {2 * g_global_padding + 2 * g_item_padding + max_width,
               2 * g_global_padding + (2 * g_item_padding + max_height) * static_cast<float>(total_count)},
      .item_size = {max_width, max_height},
      .total_count = total_count,
   };

   if (parent_id == 0) {
      m_cached_measure.emplace(measure);
   }
   return measure;
}

void TreeView::draw_level(const TreeItemId parent_id, const float offset_x, float& offset_y)
{
   const auto measure = this->get_measure(TREE_ROOT);

   const auto& children = m_state.controller->children(parent_id);
   for (const auto child : children) {
      const auto& item = m_state.controller->item(child);

      m_offset_to_item_id[offset_y - m_dimensions.y] = child;

      const Vector2 arrow_size{measure.item_size.y, measure.item_size.y};
      if (item.has_children) {
         const Vector2 arrow_pos{offset_x, offset_y + g_item_padding};

         Vector4 region{0, 0, 64, 64};
         if (!m_state.extended_items.contains(child)) {
            region = {3 * 64, 0, 64, 64};
         }

         auto& arrow_sprite = [&]() -> ui_core::SpriteInstance& {
            if (auto arrow_it = m_arrows.find(child); arrow_it != m_arrows.end()) {
               return arrow_it->second;
            }

            m_arrows.emplace(child, ui_core::SpriteInstance{
               .texture = "texture/ui_atlas.tex"_rc,
               .size = arrow_size,
               .texture_region = region,
            });
            return m_arrows.at(child);
         }();

         arrow_sprite.add(m_context, {arrow_pos, arrow_size}, m_cropping_mask);
         arrow_sprite.set_region(m_context, region);
      }

      const Vector2 icon_size{measure.item_size.y + g_item_padding, measure.item_size.y + g_item_padding};
      auto& icon_sprite = [&]() -> ui_core::SpriteInstance& {
		  if (auto icon_it = m_icons.find(child); icon_it != m_icons.end()) {
            return icon_it->second;
		  }

          m_icons.emplace(child, ui_core::SpriteInstance{
            .texture = item.icon_name,
            .size = icon_size,
            .texture_region = item.icon_region,
		  });
          return m_icons.at(child);
      }();

      const Vector2 icon_pos{offset_x + arrow_size.x + g_item_padding, offset_y + g_item_padding / 2};
      icon_sprite.add(m_context, {icon_pos, icon_size}, m_cropping_mask);

      const Vector2 label_dims{offset_x + arrow_size.x + icon_size.x + 2 * g_item_padding, offset_y + g_item_padding + measure.item_size.y};

      auto& label = [&]() -> ui_core::TextInstance& {
		  if (auto label_it = m_labels.find(child); label_it != m_labels.end()) {
            return label_it->second;
		  }

          m_labels.emplace(child, ui_core::TextInstance{
            .content = item.label,
            .typeface_name = TG_THEME_VAL(base_typeface),
            .font_size = TG_THEME_VAL(base_font_size),
            .color = TG_THEME_VAL(foreground_color),
		  });
         return m_labels.at(child);
	  }();

      label.add(m_context, label_dims, m_cropping_mask);

      offset_y += 2 * g_item_padding + measure.item_size.y;

      if (item.has_children && m_state.extended_items.contains(child)) {
         this->draw_level(child, offset_x + g_indent_width, offset_y);
      }
   }
}

void TreeView::remove_level(const TreeItemId parent_id)
{
   const auto children = m_state.controller->children(parent_id);
   for (const auto child : children) {
      const auto& item = m_state.controller->item(child);
      if (item.has_children) {
         m_arrows.at(child).remove(m_context);
         m_arrows.erase(child);
         if (m_state.extended_items.contains(child)) {
            m_state.extended_items.erase(child);
            this->remove_level(child);
         }
      }
      m_icons.at(child).remove(m_context);
      m_icons.erase(child);
      m_labels.at(child).remove(m_context);
      m_labels.erase(child);
   }
}

}// namespace triglav::desktop_ui
