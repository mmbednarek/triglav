#include "Application.hpp"

#include "GraphicsDeviceInitStage.hpp"
#include "LoadAllResourcesStage.hpp"
#include "LoadInitialResourcesStage.hpp"

#include "triglav/io/CommandLine.hpp"
#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/threading/Scheduler.hpp"
#include "triglav/threading/ThreadPool.hpp"
#include "triglav/threading/Threading.hpp"

#include <spdlog/spdlog.h>

namespace triglav::launcher {

using namespace name_literals;

Application::Application(const desktop::InputArgs& args, desktop::IDisplay& display) :
    m_display(display)
{
   // Parse program arguments
   io::CommandLine::the().parse(args.arg_count, args.args);

   // Assign ID to the main thread
   threading::set_thread_id(threading::g_mainThread);

   // Initialize global thread pool
   const auto threadCount = io::CommandLine::the().arg_int("threadCount"_name).value_or(8);
   threading::ThreadPool::the().initialize(std::clamp(threadCount, 0, 8));
}

Application::~Application()
{
   threading::ThreadPool::the().quit();
}

void Application::complete_stage()
{
   spdlog::info("launcher: completing stage {}", launch_stage_to_string(m_currentStage).to_std());
   m_currentStage = static_cast<LaunchStage>(static_cast<int>(m_currentStage) + 1);
   spdlog::info("launcher: starting stage {}", launch_stage_to_string(m_currentStage).to_std());

   switch (m_currentStage) {
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

   while (m_currentStage != LaunchStage::Complete) {
      if (m_stage != nullptr) {
         m_stage->tick();
      }

      this->tick();
   }
}

void Application::tick() const
{
   m_display.dispatch_messages();
   threading::Scheduler::the().tick();
}

bool Application::is_init_complete() const
{
   return m_currentStage == LaunchStage::Complete;
}

desktop::IDisplay& Application::display() const
{
   return m_display;
}

font::FontManger& Application::font_manager()
{
   return m_fontManager;
}

graphics_api::Instance& Application::gfx_instance() const
{
   return *m_gfxInstance;
}

graphics_api::Device& Application::gfx_device() const
{
   return *m_gfxDevice;
}

resource::ResourceManager& Application::resource_manager() const
{
   return *m_resourceManager;
}

render_core::GlyphCache& Application::glyph_cache() const
{
   return *m_glyphCache;
}

}// namespace triglav::launcher
