#include "MenuList.hpp"

#include "DesktopUI.hpp"
#include "DialogManager.hpp"

#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/Viewport.hpp"

namespace triglav::desktop_ui {

using namespace name_literals;
using namespace string_literals;

constexpr auto g_itemHMargin = 10.0f;
constexpr auto g_itemVMargin = 8.0f;
constexpr auto g_globalMargin = 6.0f;
constexpr auto g_indicatorSize = 16.0f;
constexpr auto g_indicatorMargin = 8.0f;
constexpr auto g_separatorHeight = 2.0f;

MenuList::MenuList(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
    ui_core::BaseWidget(parent),
    m_context(ctx),
    m_state(state)
{
}

Vector2 MenuList::desired_size(const Vector2 /*parentSize*/) const
{
   return this->get_measure().size;
}

void MenuList::add_to_viewport(const Vector4 dimensions, const Vector4 croppingMask)
{
   const auto measure = this->get_measure();
   m_croppingMask = croppingMask;

   m_backgroundId = m_context.viewport().add_rectangle({
      .rect = dimensions,
      .color = TG_THEME_VAL(background_color_darker),
      .borderRadius = palette::NO_COLOR,
      .borderColor = TG_THEME_VAL(active_color),
      .crop = croppingMask,
      .borderWidth = 1.0f,
   });

   const auto& items = m_state.controller->children(m_state.listName);

   float y_offset = dimensions.y + g_globalMargin;

   if (m_labels.empty()) {
      m_labels.reserve(items.size());
      for (const Name itemName : items) {
         if (itemName == "separator"_name) {
            m_separators.push_back(m_context.viewport().add_rectangle({
               .rect = {dimensions.x + g_globalMargin, y_offset + (measure.separation_height - g_separatorHeight) / 2, measure.item_size.x,
                        g_separatorHeight},
               .color = TG_THEME_VAL(active_color),
               .borderRadius = {0, 0, 0, 0},
               .borderColor = palette::NO_COLOR,
               .crop = croppingMask,
               .borderWidth = 0.0f,
            }));
            y_offset += measure.separation_height;
            continue;
         }

         m_heightToItem[y_offset] = itemName;

         const auto& item = m_state.controller->item(itemName);
         m_labels.push_back(m_context.viewport().add_text({
            .content = item.label,
            .typefaceName = TG_THEME_VAL(base_typeface),
            .fontSize = TG_THEME_VAL(base_font_size),
            .position = {dimensions.x + g_itemHMargin + g_globalMargin, y_offset + measure.text_height + g_itemVMargin},
            .color = TG_THEME_VAL(foreground_color),
            .crop = croppingMask,
         }));

         if (item.isSubmenu) {
            m_icons.push_back(m_context.viewport().add_sprite({
               .texture = "texture/ui_atlas.tex"_rc,
               .position = {dimensions.z - g_globalMargin - g_indicatorMargin - g_indicatorSize,
                            y_offset + (measure.item_size.y - g_indicatorSize) / 2},
               .size = {16, 16},
               .crop = {croppingMask},
               .textureRegion = Vector4{3 * 64, 0, 64, 64},
            }));
         }

         y_offset += measure.item_size.y;
      }
   }
}

void MenuList::remove_from_viewport()
{
   if (m_subMenu != nullptr) {
      m_state.manager->dialog_manager().close_popup(m_subMenu);
      m_subMenu = nullptr;
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
   m_context.viewport().remove_rectangle(m_backgroundId);
   if (m_hoverRectId != 0) {
      m_context.viewport().remove_rectangle(m_hoverRectId);
   }
}

void MenuList::on_event(const ui_core::Event& event)
{
   ui_core::visit_event<void>(*this, event);
}

void MenuList::on_mouse_moved(const ui_core::Event& event)
{
   const auto measure = this->get_measure();

   const auto [offset_y, new_hovered_item] = this->index_from_mouse_position(event.mousePosition);
   if (new_hovered_item == m_hoveredItem)
      return;

   m_hoveredItem = new_hovered_item;

   if (m_subMenu != nullptr) {
      m_state.manager->dialog_manager().close_popup(m_subMenu);
      m_subMenu = nullptr;
   }

   if (m_hoveredItem == 0) {
      if (m_hoverRectId != 0) {
         m_context.viewport().remove_rectangle(m_hoverRectId);
         m_hoverRectId = 0;
      }
      return;
   }

   const Vector4 dims{g_globalMargin, offset_y, measure.item_size.x, measure.item_size.y};
   if (m_hoverRectId != 0) {
      m_context.viewport().set_rectangle_dims(m_hoverRectId, dims, m_croppingMask);
   } else {
      m_hoverRectId = m_context.viewport().add_rectangle({
         .rect = dims,
         .color = TG_THEME_VAL(active_color),
         .borderRadius = {4, 4, 4, 4},
         .borderColor = palette::NO_COLOR,
         .crop = m_croppingMask,
         .borderWidth = 0.0f,
      });
   }

   auto& item = m_state.controller->item(m_hoveredItem);
   if (item.isSubmenu) {
      const Vector2 offset = m_state.screenOffset + Vector2{measure.size.x, offset_y};

      State child_state{
         .manager = m_state.manager,
         .controller = m_state.controller,
         .listName = m_hoveredItem,
         .screenOffset = offset,
      };
      const auto temporary_menu = std::make_unique<MenuList>(m_context, child_state, nullptr);
      const auto size = temporary_menu->desired_size({});

      auto& popup = m_state.manager->dialog_manager().create_popup_dialog(offset, size);
      popup.create_root_widget<MenuList>(State{child_state});
      popup.initialize();
      m_subMenu = &popup;
   }
}

void MenuList::on_mouse_released(const ui_core::Event& /*event*/, const ui_core::Event::Mouse& /*mouse*/)
{
   if (m_hoveredItem != 0) {
      m_state.controller->trigger_on_clicked(m_hoveredItem);
   }
}

std::pair<float, Name> MenuList::index_from_mouse_position(const Vector2 position) const
{
   const auto it_lb = m_heightToItem.lower_bound(position.y);
   if (it_lb == m_heightToItem.begin()) {
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
   if (m_cachedMeasure.has_value()) {
      return *m_cachedMeasure;
   }

   const auto children = m_state.controller->children(m_state.listName);
   const auto& atlas = m_context.glyph_cache().find_glyph_atlas({
      .typeface = TG_THEME_VAL(base_typeface),
      .fontSize = TG_THEME_VAL(base_font_size),
   });

   float maxHeight = 0.0f;
   float maxWidth = 0.0f;
   u32 separator_count = 0;
   for (const auto child : children) {
      if (child == "separator"_name) {
         ++separator_count;
         continue;
      }
      const auto& item = m_state.controller->item(child);
      auto measure_text = atlas.measure_text(item.label.view());
      if (item.isSubmenu) {
         measure_text.width += g_indicatorMargin + g_indicatorSize;
      }
      maxWidth = std::max(maxWidth, measure_text.width);
      maxHeight = std::max(maxHeight, measure_text.height);
   }

   const u32 item_count = static_cast<u32>(children.size()) - separator_count;

   const Vector2 item_size{2 * g_itemHMargin + maxWidth, 2 * g_itemVMargin + maxHeight};
   const float separation_height = 0.5f * item_size.y;
   m_cachedMeasure = Measure{
      .size = {2 * g_globalMargin + item_size.x,
               2 * g_globalMargin + item_size.y * static_cast<float>(item_count) + separation_height * static_cast<float>(separator_count)},
      .item_size = item_size,
      .text_height = maxHeight,
      .separation_height = separation_height,
      .separator_count = separator_count,
   };
   return *m_cachedMeasure;
}

}// namespace triglav::desktop_ui