#pragma once

#include "triglav/desktop_ui/Button.hpp"
#include "triglav/desktop_ui/DesktopUI.hpp"
#include "triglav/desktop_ui/TextInput.hpp"
#include "triglav/desktop_ui/TreeController.hpp"
#include "triglav/event/Delegate.hpp"
#include "triglav/renderer/Scene.hpp"
#include "triglav/ui_core/IWidget.hpp"

namespace triglav::desktop_ui {

class TextInput;

}
namespace triglav::editor {

class LevelEditor;
class LevelEditorSidePanel;

class TransposeInput final : public ui_core::ProxyWidget
{
 public:
   using Self = TransposeInput;

   struct State
   {
      desktop_ui::DesktopUIManager* manager{};
      LevelEditorSidePanel* side_panel;
      Color border_color{};
      float* destination;
   };

   TransposeInput(ui_core::Context& context, State state, IWidget* parent);

   void on_text_changed(StringView text) const;
   void set_content(StringView text) const;

 private:
   State m_state;
   desktop_ui::TextInput* m_text_input{};

   TG_OPT_SINK(desktop_ui::TextInput, OnTextChanged);
};

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

   void on_changed_selected_object(const renderer::SceneObject& object);
   void apply_transform() const;

   void on_object_added_to_scene(renderer::ObjectID object_id, const renderer::SceneObject& object);

 private:
   State m_state;
   desktop_ui::TreeController m_tree_controller;

   TransposeInput* m_translate_x;
   TransposeInput* m_translate_y;
   TransposeInput* m_translate_z;

   TransposeInput* m_rotate_x;
   TransposeInput* m_rotate_y;
   TransposeInput* m_rotate_z;

   TransposeInput* m_scale_x;
   TransposeInput* m_scale_y;
   TransposeInput* m_scale_z;

   ui_core::TextBox* m_mesh_label;

   Vector3 m_pending_translate;
   Vector3 m_pending_rotation;
   Vector3 m_pending_scale;

   TG_SINK(renderer::Scene, OnObjectAddedToScene);
};

}// namespace triglav::editor