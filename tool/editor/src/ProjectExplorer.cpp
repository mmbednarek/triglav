#include "ProjectExplorer.hpp"

#include "RootWindow.hpp"
#include "triglav/desktop_ui/TreeView.hpp"
#include "triglav/io/File.hpp"
#include "triglav/resource/ResourceManager.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/widget/Padding.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"
#include "triglav/ui_core/widget/ScrollBox.hpp"
#include "triglav/ui_core/widget/TextBox.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

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

void ProjectTreeController::children(const desktop_ui::TreeItemId parent, const std::function<void(desktop_ui::TreeItemId)> callback)
{
   const auto cache_it = m_cache.find(parent);
   if (cache_it != m_cache.end()) {
      for (const auto& child : cache_it->second) {
         callback(child);
      }
      return;
   }

   io::Path path = resource::PathManager::the().content_path();
   if (parent != desktop_ui::TREE_ROOT) {
      path = m_id_to_path.at(parent);
   }

   std::vector<desktop_ui::TreeItemId> children;
   for (const auto [name, is_dir] : io::list_files(path)) {
      Vector2i region;
      if (is_dir) {
         region = {5 * 18, 0};
      } else {
         region = resource_path_to_icon_offset(name.to_std());
      }
      m_id_to_item[m_top_item] = desktop_ui::TreeItem{
         .icon_name = "texture/ui_icons.tex"_rc,
         .icon_region = {region.x, region.y, 18, 18},
         .label = {name},
         .has_children = is_dir,
      };
      m_id_to_path[m_top_item] = path.sub(name.to_std());
      children.push_back(m_top_item);
      ++m_top_item;
   }

   std::ranges::sort(children, [&](const desktop_ui::TreeItemId left, const desktop_ui::TreeItemId right) {
      const auto& left_it = m_id_to_item.at(left);
      const auto& right_it = m_id_to_item.at(right);

      if (left_it.has_children != right_it.has_children) {
         return left_it.has_children;
      }

      return left_it.label < right_it.label;
   });

   for (const auto child : children) {
      callback(child);
   }

   m_cache[parent] = std::move(children);
}

const desktop_ui::TreeItem& ProjectTreeController::item(const desktop_ui::TreeItemId id)
{
   return m_id_to_item.at(id);
}

void ProjectTreeController::set_label(const desktop_ui::TreeItemId id, const StringView label)
{
   m_id_to_item[id].label = label;
}

void ProjectTreeController::remove(const desktop_ui::TreeItemId)
{
   // Unsuported
}

io::Path ProjectTreeController::path(const desktop_ui::TreeItemId id)
{
   return m_id_to_path.at(id);
}

bool ProjectTreeController::has_children(const desktop_ui::TreeItemId id) const
{
   return m_id_to_item.at(id).has_children;
}

ProjectExplorer::ProjectExplorer(ui_core::Context& context, State state, ui_core::IWidget* parent) :
    ui_core::ProxyWidget(context, parent),
    m_state(state)
{
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

   layout.create_child<ui_core::Padding>({10, 10, 10, 10})
      .create_content<ui_core::TextBox>({
         .font_size = 13,
         .typeface = "fonts/inter/regular.typeface"_rc,
         .content = "PROJECT EXPLORER",
         .color = {0.3, 0.3, 0.3, 1.0},
         .horizontal_alignment = ui_core::HorizontalAlignment::Left,
         .vertical_alignment = ui_core::VerticalAlignment::Top,
      });

   auto& scroll_rect = layout.create_child<ui_core::RectBox>({
      .color = TG_THEME_VAL(background_color_darker),
      .border_radius = {4, 4, 4, 4},
      .border_color = palette::NO_COLOR,
      .border_width = 0.0f,
   });

   auto& scroll = scroll_rect.create_content<ui_core::ScrollBox>({});

   auto& tree_view = scroll.create_content<desktop_ui::TreeView>({
      .manager = m_state.manager,
      .controller = &m_controller,
   });
   TG_CONNECT_OPT(tree_view, OnActivated, on_activated);
}

void ProjectExplorer::on_activated(const desktop_ui::TreeItemId id)
{
   if (m_controller.has_children(id))
      return;

   const auto path = m_controller.path(id);
   const auto prefix_size = resource::PathManager::the().content_path().string().size();
   const auto resource_path = path.string().substr(prefix_size + 1);
   m_state.root_window->open_asset(name_from_path(StringView{resource_path}));
}

}// namespace triglav::editor
