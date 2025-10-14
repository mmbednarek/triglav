#include "TreeView.hpp"

#include <utility>

#include "DesktopUI.hpp"

#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/Viewport.hpp"

namespace triglav::desktop_ui {

using namespace name_literals;

constexpr auto g_indentWidth = 16.0f;
constexpr auto g_globalPadding = 10.0f;
constexpr auto g_itemPadding = 6.0f;

TreeView::TreeView(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
    ui_core::BaseWidget(parent),
    m_context(ctx),
    m_state(std::move(state))
{
}

Vector2 TreeView::desired_size(Vector2 /*parentSize*/) const
{
   return this->get_measure().size;
}

void TreeView::add_to_viewport(const Vector4 dimensions, const Vector4 croppingMask)
{
   m_dimensions = dimensions;
   m_croppingMask = croppingMask;
   m_offsetToItemId.clear();
   float offset_y = g_globalPadding + dimensions.y;
   this->draw_level(TREE_ROOT, dimensions.x + g_globalPadding, offset_y);

   if (m_itemHighlight != 0) {
      const auto highlight_dims = m_highlightDims + Vector4{m_dimensions.x, m_dimensions.y, m_dimensions.z, 0};
      m_context.viewport().set_rectangle_dims(m_itemHighlight, highlight_dims, m_croppingMask);
   }
}

void TreeView::remove_from_viewport()
{
   for (const auto label : std::views::values(m_labels)) {
      m_context.viewport().remove_text(label);
   }
   m_labels.clear();
   for (const auto icon : std::views::values(m_icons)) {
      m_context.viewport().remove_sprite(icon);
   }
   m_icons.clear();
   for (const auto arrow : std::views::values(m_arrows)) {
      m_context.viewport().remove_sprite(arrow);
   }
   m_arrows.clear();
   if (m_itemHighlight != 0) {
      m_context.viewport().remove_rectangle(m_itemHighlight);
      m_itemHighlight = 0;
   }
}

void TreeView::on_event(const ui_core::Event& event)
{
   this->visit_event(event);
}

bool TreeView::on_mouse_pressed(const ui_core::Event& event, const ui_core::Event::Mouse& /*mouse*/)
{
   auto [base_offset, item_id] = this->index_from_mouse_position(event.mousePosition);
   if (item_id == 0) {
      if (m_itemHighlight != 0) {
         m_context.viewport().remove_rectangle(m_itemHighlight);
         m_itemHighlight = 0;
      }
      return false;
   }
   const auto& item = m_state.controller->item(item_id);

   const auto measure = this->get_measure();
   m_highlightDims = Vector4{0, base_offset, 0, 2 * g_itemPadding + measure.item_size.y};
   const auto highlight_dims = m_highlightDims + Vector4{m_dimensions.x, m_dimensions.y, 0, 0};
   if (m_itemHighlight != 0) {
      m_context.viewport().set_rectangle_dims(m_itemHighlight, highlight_dims, m_croppingMask);
   } else {
      m_itemHighlight = m_context.viewport().add_rectangle({
         .rect = highlight_dims,
         .color = TG_THEME_VAL(active_color),
         .borderRadius = {0, 0, 0, 0},
         .borderColor = palette::NO_COLOR,
         .crop = m_croppingMask,
         .borderWidth = 0.0f,
      });
   }

   if (item.hasChildren) {
      if (m_state.extended_items.contains(item_id)) {
         m_state.extended_items.erase(item_id);
         this->remove_level(item_id);
      } else {
         m_state.extended_items.insert(item_id);
      }
   }

   this->add_to_viewport(m_dimensions, m_croppingMask);

   return false;
}

std::pair<float, TreeItemId> TreeView::index_from_mouse_position(const Vector2 position) const
{
   const auto it_lb = m_offsetToItemId.lower_bound(position.y);
   if (it_lb == m_offsetToItemId.begin()) {
      return {0.0f, 0};
   }

   const auto it = std::prev(it_lb);
   if (position.y > it->first + this->get_measure().item_size.y + 2 * g_itemPadding) {
      return {0.0f, 0};
   }

   return *it;
}

TreeView::Measure TreeView::get_measure(const TreeItemId parentId) const
{
   if (parentId == 0 && m_cachedMeasure.has_value()) {
      return m_cachedMeasure.value();
   }

   float max_width = 0;
   float max_height = 0;
   u32 total_count = 0;

   const auto& atlas = m_context.glyph_cache().find_glyph_atlas({
      .typeface = TG_THEME_VAL(base_typeface),
      .fontSize = TG_THEME_VAL(base_font_size),
   });

   const auto& children = m_state.controller->children(parentId);
   for (const auto child : children) {
      auto item = m_state.controller->item(child);
      auto measured_label = atlas.measure_text(item.label.view());

      max_width = std::max(max_width, measured_label.width);
      max_height = std::max(max_height, measured_label.height);

      ++total_count;

      if (item.hasChildren && m_state.extended_items.contains(child)) {
         auto sub_measure = this->get_measure(child);
         max_width = std::max(max_width, sub_measure.item_size.x + g_indentWidth);
         max_height = std::max(max_height, sub_measure.item_size.y);
         total_count += sub_measure.total_count;
      }
   }

   Measure measure{
      .size = {2 * g_globalPadding + 2 * g_itemPadding + max_width,
               2 * g_globalPadding + (2 * g_itemPadding + max_width) * static_cast<float>(total_count)},
      .item_size = {max_width, max_height},
      .total_count = total_count,
   };

   if (parentId == 0) {
      m_cachedMeasure.emplace(measure);
   }
   return measure;
}

void TreeView::draw_level(const TreeItemId parentId, const float offset_x, float& offset_y)
{
   const auto measure = this->get_measure(TREE_ROOT);
   const Vector2 icon_size{measure.item_size.y, measure.item_size.y};

   const auto& children = m_state.controller->children(parentId);
   for (const auto child : children) {
      const auto& item = m_state.controller->item(child);

      m_offsetToItemId[offset_y - m_dimensions.y] = child;

      if (item.hasChildren) {
         const Vector2 arrow_pos{offset_x, offset_y + g_itemPadding};
         Vector4 region{0, 0, 64, 64};
         if (!m_state.extended_items.contains(child)) {
            region = {3 * 64, 0, 64, 64};
         }

         if (auto arrow_it = m_arrows.find(child); arrow_it != m_arrows.end()) {
            m_context.viewport().set_sprite_position(arrow_it->second, arrow_pos, m_croppingMask);
            m_context.viewport().set_sprite_texture_region(arrow_it->second, region);
         } else {
            m_arrows[child] = m_context.viewport().add_sprite({
               .texture = "texture/ui_atlas.tex"_rc,
               .position = arrow_pos,
               .size = icon_size,
               .crop = m_croppingMask,
               .textureRegion = region,
            });
         }
      }

      const Vector2 icon_pos{offset_x + icon_size.x + g_itemPadding, offset_y + g_itemPadding};
      if (auto icon_it = m_icons.find(child); icon_it != m_icons.end()) {
         m_context.viewport().set_sprite_position(icon_it->second, icon_pos, m_croppingMask);
      } else {
         m_icons[child] = m_context.viewport().add_sprite({
            .texture = item.iconName,
            .position = icon_pos,
            .size = icon_size,
            .crop = m_croppingMask,
            .textureRegion = item.iconRegion,
         });
      }

      const Vector2 label_dims{offset_x + 2 * (icon_size.x + g_itemPadding), offset_y + g_itemPadding + measure.item_size.y};
      if (auto label_it = m_labels.find(child); label_it != m_labels.end()) {
         m_context.viewport().set_text_position(label_it->second, label_dims, m_croppingMask);
      } else {
         m_labels[child] = m_context.viewport().add_text({
            .content = item.label,
            .typefaceName = TG_THEME_VAL(base_typeface),
            .fontSize = TG_THEME_VAL(base_font_size),
            .position = label_dims,
            .color = TG_THEME_VAL(foreground_color),
            .crop = m_croppingMask,
         });
      }

      offset_y += 2 * g_itemPadding + measure.item_size.y;

      if (item.hasChildren && m_state.extended_items.contains(child)) {
         this->draw_level(child, offset_x + g_indentWidth, offset_y);
      }
   }
}

void TreeView::remove_level(const TreeItemId parentId)
{
   const auto children = m_state.controller->children(parentId);
   for (const auto child : children) {
      const auto& item = m_state.controller->item(child);
      if (item.hasChildren) {
         m_context.viewport().remove_sprite(m_arrows.at(child));
         m_arrows.erase(child);
         if (m_state.extended_items.contains(child)) {
            m_state.extended_items.erase(child);
            this->remove_level(child);
         }
      }
      m_context.viewport().remove_sprite(m_icons.at(child));
      m_icons.erase(child);
      m_context.viewport().remove_text(m_labels.at(child));
      m_labels.erase(child);
   }
}

}// namespace triglav::desktop_ui
