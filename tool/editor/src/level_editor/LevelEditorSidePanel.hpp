#pragma once

#include "triglav/desktop_ui/Button.hpp"
#include "triglav/desktop_ui/DesktopUI.hpp"
#include "triglav/desktop_ui/TextInput.hpp"
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
   desktop_ui::TextInput* m_textInput{};

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

 private:
   State m_state;
   TransposeInput* m_translateX;
   TransposeInput* m_translateY;
   TransposeInput* m_translateZ;

   TransposeInput* m_rotateX;
   TransposeInput* m_rotateY;
   TransposeInput* m_rotateZ;

   TransposeInput* m_scaleX;
   TransposeInput* m_scaleY;
   TransposeInput* m_scaleZ;

   ui_core::TextBox* m_meshLabel;

   Vector3 m_pendingTranslate;
   Vector3 m_pendingRotation;
   Vector3 m_pendingScale;
};

}// namespace triglav::editor