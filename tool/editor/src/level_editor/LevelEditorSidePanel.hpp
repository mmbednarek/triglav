#pragma once

#include "triglav/desktop_ui/Button.hpp"
#include "triglav/desktop_ui/DesktopUI.hpp"
#include "triglav/desktop_ui/TextInput.hpp"
#include "triglav/renderer/Scene.hpp"
#include "triglav/ui_core/IWidget.hpp"

namespace triglav::ui_core {
class HideableWidget;
}

namespace triglav::desktop_ui {
class TextInput;
}
namespace triglav::editor {

class LevelEditor;
class TransformWidget;
class SceneView;

class LevelEditorSidePanel final : public ui_core::ProxyWidget
{
 public:
   using Self = LevelEditorSidePanel;

   struct State
   {
      desktop_ui::DesktopUIManager* manager;
      LevelEditor* editor;
   };

   LevelEditorSidePanel(ui_core::Context& context, State state, IWidget* parent);

   void on_unselected() const;
   void on_changed_selected_object(const renderer::SceneObject& object) const;
   void on_object_is_removed(renderer::ObjectID object_id) const;
   void on_changed_name(StringView name) const;
   void on_changed_mesh(StringView mesh);

 private:
   State m_state;

   SceneView* m_scene_view;
   ui_core::HideableWidget* m_object_info;
   TransformWidget* m_transform_widget;
   desktop_ui::TextInput* m_name_input;
   desktop_ui::TextInput* m_mesh_input;

   TG_OPT_NAMED_SINK(desktop_ui::TextInput, OnTextChanged, NameChange);
   TG_OPT_NAMED_SINK(desktop_ui::TextInput, OnTextChanged, MeshChange);
};

}// namespace triglav::editor