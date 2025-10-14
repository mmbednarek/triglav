#pragma once

#include "triglav/desktop_ui/DialogManager.hpp"
#include "triglav/launcher/Application.hpp"

namespace triglav::editor {

class Editor
{
 public:
   explicit Editor(launcher::Application& app);

   void init();
   int run();

 private:
   launcher::Application& m_app;
   std::unique_ptr<desktop_ui::DialogManager> m_dialogManager;
};

}// namespace triglav::editor