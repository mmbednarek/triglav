#pragma once

#include "triglav/desktop_ui/CheckBox.hpp"
#include "triglav/desktop_ui/DesktopUI.hpp"
#include "triglav/ui_core/IWidget.hpp"

namespace triglav::editor {

class LevelEditor final : public ui_core::ProxyWidget
{
 public:
   using Self = LevelEditor;
   struct State
   {
      desktop_ui::DesktopUIManager* manager;
   };

   LevelEditor(ui_core::Context& context, State state, ui_core::IWidget* parent);

 private:
   State m_state;
   desktop_ui::RadioGroup m_toolRadioGroup;
};

}// namespace triglav::editor
