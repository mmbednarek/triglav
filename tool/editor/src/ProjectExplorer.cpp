#include "ProjectExplorer.hpp"

#include "triglav/desktop_ui/TreeView.hpp"
#include "triglav/resource/ResourceManager.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"
#include "triglav/ui_core/widget/ScrollBox.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

#include <ranges>

namespace triglav::editor {

using namespace name_literals;

Vector2i resource_path_to_icon_offset(const std::string_view path)
{
   if (path.ends_with(".tex")) {
      return {6 * 18, 0};
   }
   if (path.ends_with(".mesh")) {
      return {7 * 18, 0};
   }
   if (path.ends_with("shader")) {
      return {8 * 18, 0};
   }
   if (path.ends_with(".mat")) {
      return {9 * 18, 0};
   }
   if (path.ends_with(".typeface")) {
      return {6 * 18, 18};
   }
   if (path.ends_with(".level")) {
      return {7 * 18, 18};
   }

   return {8 * 18, 18};
}

ProjectExplorer::ProjectExplorer(ui_core::Context& context, State state, ui_core::IWidget* parent) :
    ui_core::ProxyWidget(context, parent),
    m_state(state)
{
   this->init_controller();

   auto& background = this->create_content<ui_core::RectBox>({
      .color = TG_THEME_VAL(background_color_brighter),
      .border_radius = {},
      .border_color = palette::NO_COLOR,
      .border_width = 0.0f,
   });

   auto& layout = background.create_content<ui_core::VerticalLayout>({
      .padding{4, 4, 4, 4},
      .separation = 4.0f,
   });

   auto& scroll_rect = layout.create_child<ui_core::RectBox>({
      .color = TG_THEME_VAL(background_color_darker),
      .border_radius = {4, 4, 4, 4},
      .border_color = palette::NO_COLOR,
      .border_width = 0.0f,
   });

   auto& scroll = scroll_rect.create_content<ui_core::ScrollBox>({});

   scroll.create_content<desktop_ui::TreeView>({
      .manager = m_state.manager,
      .controller = &m_controller,
   });
}

void ProjectExplorer::init_controller()
{
   const auto& name_reg = m_context.resource_manager().name_registry();
   name_reg.iterate_names([this](const std::string& name) { this->add_controller_item(name); });
}

void ProjectExplorer::add_controller_item(const std::string_view path)
{
   auto pos = path.find('/');
   auto last_pos = 0ull;
   u32 item_id = desktop_ui::TREE_ROOT;
   while (pos != std::string_view::npos) {
      auto it = m_path_to_id.find(path.substr(0, pos));
      if (it == m_path_to_id.end()) {
         item_id = m_controller.add_item(item_id, desktop_ui::TreeItem{
                                                     .icon_name = "texture/ui_icons.tex"_rc,
                                                     .icon_region = {5 * 18, 0, 18, 18},
                                                     .label = {path.substr(last_pos, pos).data(), pos - last_pos},
                                                     .has_children = true,
                                                  });
         m_path_to_id[path.substr(0, pos)] = item_id;
      } else {
         item_id = it->second;
      }
      last_pos = pos + 1;
      pos = path.find('/', last_pos);
   }

   const auto icon_offset = resource_path_to_icon_offset(path);
   m_controller.add_item(item_id, desktop_ui::TreeItem{
                                     .icon_name = "texture/ui_icons.tex"_rc,
                                     .icon_region = {icon_offset.x, icon_offset.y, 18, 18},
                                     .label = {path.substr(last_pos).data(), path.size() - last_pos},
                                     .has_children = false,
                                  });
}

}// namespace triglav::editor
