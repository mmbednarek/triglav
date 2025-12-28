#include "Editor.hpp"

#include "RootWidget.hpp"

#include "triglav/Logging.hpp"
#include "triglav/String.hpp"
#include "triglav/io/Logging.hpp"

#include <chrono>
#include <thread>

namespace triglav::editor {

using namespace string_literals;
using namespace std::chrono_literals;

Editor::Editor(desktop::InputArgs& args, desktop::IDisplay& display) :
    m_app(args, display)
{
   LogManager::the().register_listener<io::StreamLogger>(io::stdout_writer());
}

void Editor::initialize()
{
   m_root_window = std::make_unique<RootWindow>(m_app.gfx_instance(), m_app.gfx_device(), m_app.display(), m_app.glyph_cache(),
                                                m_app.resource_manager());
   m_dialog_manager = std::make_unique<desktop_ui::PopupManager>(m_app.gfx_instance(), m_app.gfx_device(), m_app.glyph_cache(),
                                                                 m_app.resource_manager(), m_root_window->surface());

   m_root_widget = &m_root_window->create_root_widget<RootWidget>({
      .dialog_manager = m_dialog_manager.get(),
      .editor = this,
   });

   m_root_window->initialize();

   log_info("Initialization complete");
}

int Editor::run()
{
   m_app.initialise();

   this->initialize();

   float delta_time = 0.017f;
   while (!m_root_window->should_close() && !m_should_close) {
      auto frame_start = std::chrono::steady_clock::now();
      m_dialog_manager->tick();
      m_root_window->update();
      m_app.tick();
      m_root_widget->tick(delta_time);

      LogManager::the().flush();

      auto frame_end = std::chrono::steady_clock::now();

      const auto frame_duration = frame_end - frame_start;

      // Limit tick to 60 fps
      // if (frame_duration < 17ms) {
      //    delta_time = 0.017f;
      //    std::this_thread::sleep_for(17ms - frame_duration);
      // } else {
      //    delta_time = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(frame_duration).count()) / 1000000.0f;
      // }
   }

   m_app.gfx_device().await_all();

   return 0;
}

void Editor::close()
{
   m_should_close = true;
}

RootWindow* Editor::root_window() const
{
   return m_root_window.get();
}

}// namespace triglav::editor
