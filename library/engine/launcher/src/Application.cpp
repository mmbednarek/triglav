#include "Application.hpp"

#include "GraphicsDeviceInitStage.hpp"
#include "LoadAllResourcesStage.hpp"
#include "LoadInitialResourcesStage.hpp"

#include "triglav/io/CommandLine.hpp"
#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/threading/Scheduler.hpp"
#include "triglav/threading/ThreadPool.hpp"
#include "triglav/threading/Threading.hpp"

namespace triglav::launcher {

using namespace name_literals;

Application::Application(const desktop::InputArgs& args, desktop::IDisplay& display) :
    m_display(display)
{
   // Parse program arguments
   io::CommandLine::the().parse(args.arg_count, args.args);

   // Assign ID to the main thread
   threading::set_thread_id(threading::g_main_thread);

   // Initialize global thread pool
   const auto thread_count = io::CommandLine::the().arg_int("threadCount"_name).value_or(8);
   threading::ThreadPool::the().initialize(std::clamp(thread_count, 0, 8));
}

Application::~Application()
{
   threading::ThreadPool::the().quit();
}

void Application::complete_stage()
{
   log_info("completing stage {}", launch_stage_to_string(m_current_stage).to_std());
   m_current_stage = static_cast<LaunchStage>(static_cast<int>(m_current_stage) + 1);
   log_info("starting stage {}", launch_stage_to_string(m_current_stage).to_std());

   switch (m_current_stage) {
#define TG_STAGE(name)                                \
   case LaunchStage::name: {                          \
      m_stage = std::make_unique<name##Stage>(*this); \
      break;                                          \
   }
      TG_LAUNCH_STAGES
#undef TG_STAGE
   default:
      m_stage.reset();
      break;
   }
}

void Application::intitialize()
{
   this->complete_stage();

   while (m_current_stage != LaunchStage::Complete) {
      if (m_stage != nullptr) {
         m_stage->tick();
      }

      this->tick();

      LogManager::the().flush();
   }
}

void Application::tick() const
{
   m_display.dispatch_messages();
   threading::Scheduler::the().tick();
}

bool Application::is_init_complete() const
{
   return m_current_stage == LaunchStage::Complete;
}

desktop::IDisplay& Application::display() const
{
   return m_display;
}

font::FontManger& Application::font_manager()
{
   return m_font_manager;
}

graphics_api::Instance& Application::gfx_instance() const
{
   return *m_gfx_instance;
}

graphics_api::Device& Application::gfx_device() const
{
   return *m_gfx_device;
}

resource::ResourceManager& Application::resource_manager() const
{
   return *m_resource_manager;
}

render_core::GlyphCache& Application::glyph_cache() const
{
   return *m_glyph_cache;
}

}// namespace triglav::launcher
