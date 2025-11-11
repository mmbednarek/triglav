#pragma once
#include "triglav/desktop_ui/DesktopUI.hpp"
#include "triglav/ui_core/IWidget.hpp"

namespace triglav::editor {

class LevelEditorSidePanel final : public ui_core::ProxyWidget
{
 public:
   struct State
   {
      desktop_ui::DesktopUIManager* manager;
   };

   LevelEditorSidePanel(ui_core::Context& context, State state, IWidget* parent);

 private:
   State m_state;
};

}// namespace triglav::editor