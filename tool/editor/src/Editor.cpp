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
   m_dialogManager =
      std::make_unique<desktop_ui::DialogManager>(m_app.gfx_instance(), m_app.gfx_device(), m_app.display(), m_app.glyph_cache(),
                                                  m_app.resource_manager(), Vector2{1280, 720}, "Triglav Editor"_strv);

   m_rootWidget = &m_dialogManager->root().create_root_widget<RootWidget>({
      .dialogManager = m_dialogManager.get(),
      .editor = this,
   });

   m_dialogManager->root().initialize();
}

int Editor::run()
{
   m_app.intitialize();

   this->initialize();

   while (!m_dialogManager->should_close() && !m_shouldClose) {
      m_dialogManager->tick();
      m_app.tick();
   }

   m_app.gfx_device().await_all();

   return 0;
}

void Editor::close()
{
   m_shouldClose = true;
}

}// namespace triglav::editor
