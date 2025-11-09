#include "Editor.hpp"

#include "RootWidget.hpp"

#include "triglav/Logging.hpp"
#include "triglav/String.hpp"

#include <chrono>
#include <thread>

namespace triglav::editor {

using namespace string_literals;
using namespace std::chrono_literals;

Editor::Editor(desktop::InputArgs& args, desktop::IDisplay& display) :
    m_app(args, display)
{
   LogManager::the().register_listener<StdOutLogger>();
}

void Editor::initialize()
{
   m_rootWindow = std::make_unique<RootWindow>(m_app.gfx_instance(), m_app.gfx_device(), m_app.display(), m_app.glyph_cache(),
                                               m_app.resource_manager());
   m_dialogManager = std::make_unique<desktop_ui::PopupManager>(m_app.gfx_instance(), m_app.gfx_device(), m_app.glyph_cache(),
                                                                m_app.resource_manager(), m_rootWindow->surface());

   m_rootWidget = &m_rootWindow->create_root_widget<RootWidget>({
      .dialogManager = m_dialogManager.get(),
      .editor = this,
   });

   m_rootWindow->initialize();

   log_info("Initialization complete");
}

int Editor::run()
{
   m_app.intitialize();

   this->initialize();

   float delta_time = 0.017f;
   while (!m_rootWindow->should_close() && !m_shouldClose) {
      auto frame_start = std::chrono::steady_clock::now();
      m_dialogManager->tick();
      m_rootWindow->update();
      m_app.tick();
      m_rootWidget->tick(delta_time);

      LogManager::the().flush();

      auto frame_end = std::chrono::steady_clock::now();

      const auto frame_duration = frame_end - frame_start;

      // Limit tick to 60 fps
      if (frame_duration < 17ms) {
         delta_time = 0.017f;
         std::this_thread::sleep_for(17ms - frame_duration);
      } else {
         delta_time = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(frame_duration).count()) / 1000000.0f;
      }
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
