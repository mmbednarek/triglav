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

ProjectExplorer::ProjectExplorer(ui_core::Context& context, State state, ui_core::IWidget* parent) :
    ui_core::ProxyWidget(context, parent),
    m_state(state)
{
   this->init_controller();

   auto& background = this->create_content<ui_core::RectBox>({
      .color = TG_THEME_VAL(background_color_brighter),
      .borderRadius = {},
      .borderColor = palette::NO_COLOR,
      .borderWidth = 0.0f,
   });

   auto& layout = background.create_content<ui_core::VerticalLayout>({
      .padding{2.0f, 2.0f, 2.0f, 2.0f},
      .separation = 4.0f,
   });

   auto& scrollRect = layout.create_child<ui_core::RectBox>({
      .color = TG_THEME_VAL(background_color_darker),
      .borderRadius = {2.0f, 2.0f, 2.0f, 2.0f},
      .borderColor = palette::NO_COLOR,
      .borderWidth = 0.0f,
   });

   auto& scroll = scrollRect.create_content<ui_core::ScrollBox>({});

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

void ProjectExplorer::add_controller_item(std::string_view path)
{
   auto pos = path.find('/');
   auto last_pos = 0ull;
   u32 item_id = desktop_ui::TREE_ROOT;
   while (pos != std::string_view::npos) {
      auto it = m_pathToId.find(path.substr(0, pos));
      if (it == m_pathToId.end()) {
         item_id = m_controller.add_item(item_id, desktop_ui::TreeItem{
                                                     .iconName = "texture/ui_atlas.tex"_rc,
                                                     .iconRegion = {0, 128, 64, 64},
                                                     .label = {path.substr(last_pos, pos).data(), pos - last_pos},
                                                     .hasChildren = true,
                                                  });
         m_pathToId[path.substr(0, pos)] = item_id;
      } else {
         item_id = it->second;
      }
      last_pos = pos + 1;
      pos = path.find('/', last_pos);
   }

   m_controller.add_item(item_id, desktop_ui::TreeItem{
                                     .iconName = "texture/ui_atlas.tex"_rc,
                                     .iconRegion = {64, 128, 64, 64},
                                     .label = {path.substr(last_pos).data(), path.size() - last_pos},
                                     .hasChildren = false,
                                  });
}

}// namespace triglav::editor
