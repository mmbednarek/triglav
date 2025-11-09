#pragma once

#include "RootWindow.hpp"

#include "triglav/Logging.hpp"
#include "triglav/desktop_ui/PopupManager.hpp"
#include "triglav/launcher/Application.hpp"

namespace triglav::editor {

class RootWidget;

class Editor
{
   TG_DEFINE_LOG_CATEGORY(Editor)
 public:
   explicit Editor(desktop::InputArgs& args, desktop::IDisplay& display);

   void initialize();
   int run();
   void close();

   [[nodiscard]] RootWindow* root_window() const;

 private:
   launcher::Application m_app;
   std::unique_ptr<RootWindow> m_rootWindow;
   std::unique_ptr<desktop_ui::PopupManager> m_dialogManager;
   RootWidget* m_rootWidget{};
   bool m_shouldClose = false;
};

}// namespace triglav::editor