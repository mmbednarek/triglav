#include "MenuBar.hpp"

#include "DesktopUI.hpp"
#include "DialogManager.hpp"
#include "MenuList.hpp"

#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/Viewport.hpp"

namespace triglav::desktop_ui {

using namespace name_literals;

constexpr auto g_itemHMargin = 10.0f;
constexpr auto g_itemVMargin = 8.0f;
constexpr auto g_globalMargin = 6.0f;

MenuBar::MenuBar(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
    ui_core::BaseWidget(parent),
    m_context(ctx),
    m_state(state)
{
}

Vector2 MenuBar::desired_size(Vector2 /*parentSize*/) const
{
   return this->get_measure().size;
}

void MenuBar::add_to_viewport(const Vector4 dimensions, const Vector4 croppingMask)
{
   const auto measure = this->get_measure();
   m_dimensions = dimensions;
   m_croppingMask = croppingMask;

   m_backgroundId = m_context.viewport().add_rectangle({
      .rect = dimensions,
      .color = TG_THEME_VAL(background_color),
      .borderRadius = {0, 0, 0, 0},
      .borderColor = palette::TRANSPARENT,
      .crop = croppingMask,
      .borderWidth = 0.0f,
   });

   const auto& items = m_state.controller->children("root"_name);

   float x_offset = dimensions.x + g_globalMargin;

   if (m_labels.empty()) {
      m_labels.reserve(items.size());
      for (const Name itemName : items) {
         m_offsetToItem[x_offset] = itemName;

         const auto& item = m_state.controller->item(itemName);
         m_labels.push_back(m_context.viewport().add_text({
            .content = item.label,
            .typefaceName = TG_THEME_VAL(base_typeface),
            .fontSize = TG_THEME_VAL(base_font_size),
            .position = {x_offset + g_itemHMargin, dimensions.y + g_globalMargin + g_itemVMargin + measure.item_height},
            .color = TG_THEME_VAL(foreground_color),
            .crop = croppingMask,
         }));

         x_offset += 2 * g_itemHMargin + measure.item_widths.at(itemName);
      }
   }
}

void MenuBar::remove_from_viewport()
{
   m_context.viewport().remove_rectangle(m_backgroundId);
   for (const auto label : m_labels) {
      m_context.viewport().remove_text(label);
   }
   if (m_hoverRectId != 0) {
      m_context.viewport().remove_rectangle(m_hoverRectId);
   }
}

void MenuBar::on_event(const ui_core::Event& event)
{
   this->visit_event(event);
}

void MenuBar::on_mouse_moved(const ui_core::Event& event)
{
   const auto [offset_x, new_hovered_item] = this->index_from_mouse_position(event.mousePosition);
   if (new_hovered_item == m_hoveredItem)
      return;

   m_hoveredItem = new_hovered_item;
   m_hoveredItemOffset = offset_x;

   this->close_submenu();
}

void MenuBar::on_mouse_pressed(const ui_core::Event& /*event*/, const ui_core::Event::Mouse& /*mouse*/)
{
   const auto measure = this->get_measure();

   this->close_submenu();

   const Vector4 dims{m_hoveredItemOffset, g_globalMargin, 2 * g_itemHMargin + measure.item_widths.at(m_hoveredItem),
                      2 * g_itemVMargin + measure.item_height};
   if (m_hoverRectId != 0) {
      m_context.viewport().set_rectangle_dims(m_hoverRectId, dims, m_croppingMask);
   } else {
      m_hoverRectId = m_context.viewport().add_rectangle({
         .rect = dims,
         .color = TG_THEME_VAL(active_color),
         .borderRadius = {4, 4, 4, 4},
         .borderColor = palette::TRANSPARENT,
         .crop = m_croppingMask,
         .borderWidth = 0.0f,
      });
   }

   const Vector2 offset{m_hoveredItemOffset, m_dimensions.y + m_dimensions.w};
   MenuList::State child_state{
      .manager = m_state.manager,
      .controller = m_state.controller,
      .listName = m_hoveredItem,
      .screenOffset = offset,
   };
   const auto temporary_menu = std::make_unique<MenuList>(m_context, child_state, nullptr);
   const auto size = temporary_menu->desired_size({});

   auto& popup = m_state.manager->dialog_manager().create_popup_dialog(offset, size);
   popup.create_root_widget<MenuList>(MenuList::State{child_state});
   popup.initialize();
   m_subMenu = &popup;
}

void MenuBar::on_mouse_left(const ui_core::Event& /*event*/)
{
   if (m_subMenu != nullptr) {
      m_state.manager->dialog_manager().close_popup(m_subMenu);
      m_subMenu = nullptr;
   }

   if (m_hoverRectId != 0) {
      m_context.viewport().remove_rectangle(m_hoverRectId);
      m_hoverRectId = 0;
   }
}

std::pair<float, Name> MenuBar::index_from_mouse_position(const Vector2 position) const
{
   const auto it_lb = m_offsetToItem.lower_bound(position.x);
   if (it_lb == m_offsetToItem.begin()) {
      return {0.0f, 0};
   }

   const auto it = std::prev(it_lb);
   if (position.x > (it->first + 2 * g_itemHMargin + this->get_measure().item_widths.at(it->second))) {
      return {0.0f, 0};
   }

   return *it;
}

void MenuBar::close_submenu()
{
   if (m_hoverRectId != 0) {
      m_context.viewport().remove_rectangle(m_hoverRectId);
      m_hoverRectId = 0;
   }

   if (m_subMenu != nullptr) {
      m_state.manager->dialog_manager().close_popup(m_subMenu);
      m_subMenu = nullptr;
   }
}

MenuBar::Measure MenuBar::get_measure() const
{
   if (m_cachedMeasure.has_value()) {
      return *m_cachedMeasure;
   }

   const auto children = m_state.controller->children("root"_name);
   const auto& atlas = m_context.glyph_cache().find_glyph_atlas({
      .typeface = TG_THEME_VAL(base_typeface),
      .fontSize = TG_THEME_VAL(base_font_size),
   });

   float max_height = 0.0f;
   float total_width = 0.0f;
   std::map<Name, float> item_widths;
   for (const auto child : children) {
      assert(child != "separator"_name && "Separators are not support for top level menu bar");

      const auto& item = m_state.controller->item(child);
      assert(item.isSubmenu && "All top level items must be submenus");

      auto measure_text = atlas.measure_text(item.label.view());
      item_widths.emplace(child, measure_text.width);
      max_height = std::max(max_height, measure_text.height);
      total_width += measure_text.width + 2 * g_itemHMargin;
   }

   m_cachedMeasure = Measure{
      .size = {2 * g_globalMargin + total_width, 2 * g_globalMargin + 2 * g_itemVMargin + max_height},
      .item_widths = std::move(item_widths),
      .item_height = max_height,
   };
   return *m_cachedMeasure;
}

}// namespace triglav::desktop_ui