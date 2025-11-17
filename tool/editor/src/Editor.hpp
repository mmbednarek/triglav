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
   std::unique_ptr<RootWindow> m_root_window;
   std::unique_ptr<desktop_ui::PopupManager> m_dialog_manager;
   RootWidget* m_root_widget{};
   bool m_should_close = false;
};

}// namespace triglav::editor