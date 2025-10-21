#include "Editor.hpp"

#include "RootWidget.hpp"

#include "triglav/String.hpp"

namespace triglav::editor {

using namespace string_literals;

Editor::Editor(desktop::InputArgs& args, desktop::IDisplay& display) :
    m_app(args, display)
{
}

void Editor::initialize()
{
   m_rootWindow = std::make_unique<RootWindow>(m_app.gfx_instance(), m_app.gfx_device(), m_app.display(), m_app.glyph_cache(),
                                               m_app.resource_manager());
   m_dialogManager = std::make_unique<desktop_ui::PopupManager>(m_app.gfx_instance(), m_app.gfx_device(), m_app.display(),
                                                                m_app.glyph_cache(), m_app.resource_manager(), m_rootWindow->surface());

   m_rootWidget = &m_rootWindow->create_root_widget<RootWidget>({
      .dialogManager = m_dialogManager.get(),
      .editor = this,
   });

   m_rootWindow->initialize();
}

int Editor::run()
{
   m_app.intitialize();

   this->initialize();

   while (!m_rootWindow->should_close() && !m_shouldClose) {
      m_dialogManager->tick();
      m_rootWindow->update();
      m_app.tick();
   }

   m_app.gfx_device().await_all();

   return 0;
}

void Editor::close()
{
   m_shouldClose = true;
}

RootWindow* Editor::root_window() const
{
   return m_rootWindow.get();
}

}// namespace triglav::editor
