#pragma once

#include "triglav/desktop_ui/Button.hpp"
#include "triglav/desktop_ui/DesktopUI.hpp"
#include "triglav/renderer/Scene.hpp"
#include "triglav/ui_core/IWidget.hpp"

namespace triglav::desktop_ui {

class TextInput;

}
namespace triglav::editor {

class LevelEditor;

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

   void on_changed_selected_object(const renderer::SceneObject& object) const;
   void apply_position(desktop::MouseButton mouse_button) const;

 private:
   State m_state;
   desktop_ui::TextInput* m_translateX;
   desktop_ui::TextInput* m_translateY;
   desktop_ui::TextInput* m_translateZ;

   desktop_ui::TextInput* m_rotateX;
   desktop_ui::TextInput* m_rotateY;
   desktop_ui::TextInput* m_rotateZ;

   desktop_ui::TextInput* m_scaleX;
   desktop_ui::TextInput* m_scaleY;
   desktop_ui::TextInput* m_scaleZ;

   TG_OPT_SINK(desktop_ui::Button, OnClick);
};

}// namespace triglav::editor