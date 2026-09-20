#include "LevelEditorSidePanel.hpp"

#include "../RootWindow.hpp"
#include "LevelEditor.hpp"
#include "SceneView.hpp"
#include "SetTransformAction.hpp"

#include "triglav/Format.hpp"
#include "triglav/desktop_ui/MetaWidget.hpp"
#include "triglav/desktop_ui/Splitter.hpp"
#include "triglav/ui_core/widget/HideableWidget.hpp"
#include "triglav/ui_core/widget/HorizontalLayout.hpp"
#include "triglav/ui_core/widget/Image.hpp"
#include "triglav/ui_core/widget/Padding.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"
#include "triglav/ui_core/widget/TextBox.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

#include <glm/gtx/euler_angles.hpp>

namespace triglav::editor {

using namespace name_literals;

class PanelHeader final : public desktop_ui::DesktopProxyWidget
{
 public:
   struct State
   {
      String label;
      Vector4 icon_region;
   };

   PanelHeader(ui_core::Context& context, State state, IWidget* parent) :
       desktop_ui::DesktopProxyWidget(context, parent),
       m_state(std::move(state))
   {
      m_rect_box = &this->create_content<ui_core::RectBox>({
         .color = Vector4{0.16f, 0.16f, 0.16f, 1.0f},
         .border_radius = {8.0f, 8.0f, 0.0f, 0.0f},
         .border_color = palette::NO_COLOR,
         .border_width = 0.0f,
      });

      auto& hor_layout = m_rect_box->create_content<ui_core::HorizontalLayout>({
         .padding = {8.0f, 8.0f, 8.0f, 8.0f},
         .separation = 8.0f,
      });

      auto& hide_button = hor_layout.create_child<ui_core::Button>({});
      m_hide_sink = hide_button.event_OnClick.connect<&PanelHeader::on_clicked_hide>(*this);

      m_hide_icon = &hide_button.create_content<ui_core::Image>({
         .texture = desktop_ui::ICON_ATLAS,
         .max_size = Vector2{18, 18},
         .region = desktop_ui::icon_region(desktop_ui::AtlasIcon::ArrowDown),
      });

      hor_layout.create_child<ui_core::Image>({
         .texture = "editor/texture/ui_icons.tex"_rc,
         .max_size = Vector2{18, 18},
         .region = m_state.icon_region,
      });

      hor_layout.create_child<ui_core::TextBox>({
         .font_size = 13,
         .typeface = "engine/fonts/inter/regular.typeface"_rc,
         .content = m_state.label,
         .color = {0.6, 0.6, 0.6, 1.0},
         .horizontal_alignment = ui_core::HorizontalAlignment::Left,
         .vertical_alignment = ui_core::VerticalAlignment::Center,
      });
   }

   void on_clicked_hide(const desktop::MouseButton /*mouse_button*/)
   {
      if (m_hideable_widget->state().is_hidden) {
         m_hideable_widget->set_is_hidden(false);
         m_hide_icon->set_region(desktop_ui::icon_region(desktop_ui::AtlasIcon::ArrowDown));
         m_rect_box->set_border_radius({8.0f, 8.0f, 0.0f, 0.0f});
      } else {
         m_hideable_widget->set_is_hidden(true);
         m_hide_icon->set_region(desktop_ui::icon_region(desktop_ui::AtlasIcon::ArrowRight));
         m_rect_box->set_border_radius({8.0f, 8.0f, 8.0f, 8.0f});
      }
   }

   ui_core::HideableWidget* m_hideable_widget = nullptr;

 private:
   State m_state;
   Sink m_hide_sink;
   ui_core::Image* m_hide_icon;
   ui_core::RectBox* m_rect_box;
};

namespace {

Vector4 component_class_to_icon(const Name meta_type)
{
   switch (meta_type) {
   case "triglav::Transform3D"_name:
      return {5 * 18, 2 * 18, 18, 18};
   case "triglav::world::Mesh"_name:
      return {7 * 18, 0 * 18, 18, 18};
   case "triglav::world::Tag"_name:
      return {8 * 18, 2 * 18, 18, 18};
   case "triglav::world::EntityLabel"_name:
      return {9 * 18, 2 * 18, 18, 18};
   case "triglav::world::TerrainComponent"_name:
      return {5 * 18, 3 * 18, 18, 18};
   default:
      return {5 * 18, 1 * 18, 18, 18};
   }
}

}// namespace

class ComponentProvider : public desktop_ui::IMetaProvider
{
 public:
   ComponentProvider(world::Level* level, const world::EntityID entity_id, const world::ComponentID component_id,
                     const Name meta_type_name) :
       m_level(level),
       m_entity_id(entity_id),
       m_component_id(component_id),
       m_meta_type_name(meta_type_name)
   {
   }

   [[nodiscard]] meta::ClassRef get_reference() const override
   {
      void* component = m_level->component_raw(m_entity_id, m_component_id);
      return meta::ClassRef{component, m_meta_type_name};
   }

   void mutate() override
   {
      m_level->component_mut_raw(m_entity_id, m_component_id);
   }

 private:
   world::Level* m_level;
   world::EntityID m_entity_id;
   world::ComponentID m_component_id;
   Name m_meta_type_name;
};

class ComponentView : public desktop_ui::DesktopProxyWidget
{
 public:
   struct State
   {
      world::EntityID entity_id;
      world::Level* level;
   };

   ComponentView(ui_core::Context& context, State state, IWidget* parent) :
       DesktopProxyWidget(context, parent),
       m_state(state)
   {
      auto& vert_layout = this->create_content<ui_core::VerticalLayout>({
         .padding = {6, 6, 6, 6},
         .separation = 8.0f,
      });

      if (m_state.level == nullptr)
         return;

      m_state.level->iterate_components(m_state.entity_id, [&](const world::ComponentID component_id) {
         auto& comp_layout = vert_layout.create_child<ui_core::VerticalLayout>({
            .padding = {},
            .separation = 0.0f,
         });

         const auto& info = world::ComponentManager::the().info_by_id(component_id);

         auto& panel =
            comp_layout.create_child<PanelHeader>({.label = info.name, .icon_region = component_class_to_icon(info.component_class)});

         auto& hideable = comp_layout.create_child<ui_core::HideableWidget>({
            .is_hidden = false,
         });

         panel.m_hideable_widget = &hideable;

         auto& rect_box = hideable.create_content<ui_core::RectBox>({
            .color = Vector4{0.14f, 0.14f, 0.14f, 1.0f},
            .border_radius = {0, 0, 8, 8},
            .border_color = palette::NO_COLOR,
            .border_width = 0.0f,
         });

         rect_box.create_content<ui_core::Padding>({6.0f, 6.0f, 6.0f, 6.0f})
            .create_content<desktop_ui::MetaWidget>({
               .meta_type = info.component_class,
               .provider = std::make_unique<ComponentProvider>(m_state.level, m_state.entity_id, component_id, info.component_class),
            });
      });
   }

 private:
   State m_state;
};

LevelEditorSidePanel::LevelEditorSidePanel(ui_core::Context& context, State state, IWidget* parent) :
    desktop_ui::DesktopProxyWidget(context, parent),
    m_state(state)
{
   auto& split = this->create_content<desktop_ui::Splitter>({
      .offset = 300,
      .axis = ui_core::Axis::Vertical,
      .offset_type = desktop_ui::SplitterOffsetType::Preceeding,
   });

   m_scene_view = &split.create_preceding<SceneView>({
      .editor = m_state.editor,
   });

   m_object_info = &split
                       .create_following<ui_core::RectBox>({
                          .color = TG_THEME_VAL(background_color_brighter),
                          .border_radius = {0, 0, 0, 0},
                          .border_color = palette::NO_COLOR,
                          .border_width = 0.0f,
                       })
                       .create_content<ui_core::HideableWidget>({
                          .is_hidden = true,
                       });
}

void LevelEditorSidePanel::on_unselected() const
{
   if (!m_object_info->state().is_hidden) {
      m_object_info->set_is_hidden(true);
   }
}

void LevelEditorSidePanel::on_changed_selected_object(const world::EntityID entity_id) const
{
   m_object_info->remove_from_viewport();
   m_object_info->create_content<ComponentView>({
      .entity_id = entity_id,
      .level = &m_state.editor->level(),
   });
   m_object_info->set_is_hidden(false);

   m_scene_view->update_selected_item();
}

void LevelEditorSidePanel::on_object_is_removed(const world::EntityID object_id) const
{
   m_scene_view->on_removed_entity(object_id);
}

void LevelEditorSidePanel::on_changed_name(const StringView name) const
{
   m_state.editor->set_selected_name(name);
}

void LevelEditorSidePanel::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   m_dimensions = dimensions;
   m_cropping_mask = cropping_mask;
   DesktopProxyWidget::add_to_viewport(dimensions, cropping_mask);
}

void LevelEditorSidePanel::on_child_state_changed(IWidget& /*widget*/)
{
   m_content->add_to_viewport(m_dimensions, m_cropping_mask);
}

void LevelEditorSidePanel::on_changed_mesh(StringView /*mesh*/)
{
   // TODO: Implement
}

}// namespace triglav::editor
