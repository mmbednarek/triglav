#include "MenuList.hpp"

#include "DesktopUI.hpp"
#include "DialogManager.hpp"

#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/Viewport.hpp"

namespace triglav::desktop_ui {

using namespace name_literals;
using namespace string_literals;

constexpr auto g_leftMargin = 10.0f;
constexpr auto g_globalMargin = 6.0f;

MenuList::MenuList(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
    ui_core::BaseWidget(parent),
    m_context(ctx),
    m_state(std::move(state))
{
}

Vector2 MenuList::desired_size(const Vector2 parentSize) const
{
   return parentSize;
}

void MenuList::add_to_viewport(const Vector4 dimensions, const Vector4 croppingMask)
{
   m_backgroundId = m_context.viewport().add_rectangle({
      .rect = dimensions,
      .color = {0.05, 0.05, 0.05, 1},
      .borderRadius = {0, 0, 0, 0},
      .borderColor = {0.1, 0.1, 0.1, 1},
      .crop = croppingMask,
      .borderWidth = 1.0f,
   });

   const auto& items = m_state.controller->children(m_state.listName);

   m_sizePerItem = (dimensions.w - 2 * g_globalMargin) / static_cast<float>(items.size());
   m_croppingMask = croppingMask;

   const auto& atlas = m_context.glyph_cache().find_glyph_atlas({
      .typeface = "cantarell.typeface"_rc,
      .fontSize = 15,
   });
   const auto measure = atlas.measure_text("0"_strv);
   const auto top_margin = (m_sizePerItem - measure.height) / 2;

   float y_offset = dimensions.y + g_globalMargin + measure.height + top_margin;

   if (m_labels.empty()) {
      m_labels.reserve(items.size());
      for (const Name itemName : items) {
         const auto& item = m_state.controller->item(itemName);
         m_labels.push_back(m_context.viewport().add_text({
            .content = item.label,
            .typefaceName = "cantarell.typeface"_rc,
            .fontSize = 15,
            .position = {dimensions.x + g_leftMargin + g_globalMargin, y_offset},
            .color = {1, 1, 1, 1},
            .crop = croppingMask,
         }));
         y_offset += m_sizePerItem;
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
   m_context.viewport().remove_rectangle(m_backgroundId);
   if (m_hoverRectId != 0) {
      m_context.viewport().remove_rectangle(m_hoverRectId);
      m_hoverRectId = 0;
   }
}

void MenuList::on_event(const ui_core::Event& event)
{
   const auto& items = m_state.controller->children(m_state.listName);

   if (event.eventType == ui_core::Event::Type::MouseMoved) {
      const auto index = this->index_from_mouse_position(event.mousePosition);
      if (index != m_hoverIndex) {
         if (index >= items.size()) {
            m_hoverIndex = ~0;
            if (m_hoverRectId != 0) {
               m_context.viewport().remove_rectangle(m_hoverRectId);
               m_hoverRectId = 0;
            }
         } else {
            m_hoverIndex = index;
            const Vector4 dims{g_globalMargin, g_globalMargin + static_cast<float>(m_hoverIndex) * m_sizePerItem, 128, 32};
            if (m_hoverRectId != 0) {
               m_context.viewport().set_rectangle_dims(m_hoverRectId, dims, m_croppingMask);
            } else {
               m_hoverRectId = m_context.viewport().add_rectangle({
                  .rect = dims,
                  .color = {0.08, 0.08, 0.08, 1},
                  .borderRadius = {4, 4, 4, 4},
                  .borderColor = {0, 0, 0, 0},
                  .crop = m_croppingMask,
                  .borderWidth = 0.0f,
               });
            }

            if (m_subMenu != nullptr) {
               m_state.manager->dialog_manager().close_popup(m_subMenu);
               m_subMenu = nullptr;
            }

            const auto item_name = items.at(m_hoverIndex);
            auto& item = m_state.controller->item(item_name);
            if (item.isSubmenu) {
               const auto size = MenuList::calculate_size(*m_state.controller, item_name);
               const Vector2 offset = m_state.screenOffset + Vector2{2 * g_globalMargin + 128, g_globalMargin + 32 * m_hoverIndex};

               auto& popup = m_state.manager->dialog_manager().create_popup_dialog(offset, size);
               popup.create_root_widget<MenuList>({
                  .manager = m_state.manager,
                  .controller = m_state.controller,
                  .listName = item_name,
                  .screenOffset = offset,
               });
               popup.initialize();
               m_subMenu = &popup;
            }
         }
      }
   } else if (event.eventType == ui_core::Event::Type::MouseReleased) {
      if (m_hoverIndex != ~0u) {
         m_state.controller->trigger_on_clicked(items[m_hoverIndex]);
      }
   }
}

Vector2 MenuList::calculate_size(const MenuController& controller, const Name list_name)
{
   return {2 * g_globalMargin + 128, 2 * g_globalMargin + 32 * controller.children(list_name).size()};
}

u32 MenuList::index_from_mouse_position(const Vector2 position) const
{
   const auto indexFP = (position.y - g_globalMargin) / m_sizePerItem;
   return indexFP >= 0.0f ? static_cast<u32>(indexFP) : ~0u;
}

}// namespace triglav::desktop_ui