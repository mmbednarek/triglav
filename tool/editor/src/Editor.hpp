#pragma once

#include "triglav/desktop_ui/PopupManager.hpp"
#include "triglav/launcher/Application.hpp"

namespace triglav::editor {

class RootWidget;

class Editor
{
 public:
   explicit Editor(desktop::InputArgs& args, desktop::IDisplay& display);

   void initialize();
   int run();
   void close();

 private:
   launcher::Application m_app;
   std::unique_ptr<desktop_ui::Dialog> m_dialog;
   std::unique_ptr<desktop_ui::PopupManager> m_dialogManager;
   RootWidget* m_rootWidget{};
   bool m_shouldClose = false;
};

}// namespace triglav::editor