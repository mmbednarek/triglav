#include "Editor.hpp"

namespace triglav::editor {

Editor::Editor(launcher::Application& app) :
    m_app(app)
{
}

void Editor::init()
{
   m_dialogManager = std::make_unique<desktop_ui::DialogManager>(m_app.gfx_instance(), m_app.gfx_device(), m_app.display(),
                                                                 m_app.glyph_cache(), m_app.resource_manager(), Vector2{800, 600});
}

int Editor::run()
{
   // Initialize engine
   while (!m_app.is_init_complete()) {
      m_app.tick();
   }

   return 0;
}

}// namespace triglav::editor
