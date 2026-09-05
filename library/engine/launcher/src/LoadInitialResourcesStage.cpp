#include "LoadInitialResourcesStage.hpp"

#include "Application.hpp"

#include "triglav/render_core/GlyphCache.hpp"

namespace triglav::launcher {

using namespace name_literals;

LoadInitialResourcesStage::LoadInitialResourcesStage(Application& app) :
    IStage(app)
{
   engine::Engine::the().initialize(app.gfx_device());
   TG_CONNECT_OPT(engine::Engine::the(), OnEngineReady, on_engine_ready);

   app.m_glyph_cache = std::make_unique<render_core::GlyphCache>(*app.m_gfx_device, engine::Engine::the().resource_manager());
}

void LoadInitialResourcesStage::tick()
{
   if (m_completed.load()) {
      m_application.complete_stage();
   }
}

void LoadInitialResourcesStage::on_engine_ready()
{
   m_completed.store(true);
}

}// namespace triglav::launcher