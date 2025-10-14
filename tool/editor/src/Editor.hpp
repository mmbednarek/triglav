#pragma once

#include "triglav/desktop_ui/DialogManager.hpp"
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
   std::unique_ptr<desktop_ui::DialogManager> m_dialogManager;
   RootWidget* m_rootWidget{};
   bool m_shouldClose = false;
};

}// namespace triglav::editor