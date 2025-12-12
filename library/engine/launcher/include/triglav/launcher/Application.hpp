#pragma once
#include "Launcher.hpp"

#include "triglav/Logging.hpp"
#include "triglav/desktop/Entrypoint.hpp"
#include "triglav/desktop/IDisplay.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/Instance.hpp"
#include "triglav/resource/ResourceManager.hpp"

#include <memory>

namespace triglav::render_core {
class GlyphCache;
}

namespace triglav::launcher {

class Application
{
   TG_DEFINE_LOG_CATEGORY(AppLauncher)

#define TG_STAGE(name) friend name##Stage;
   TG_LAUNCH_STAGES
#undef TG_STAGE
 public:
   Application(const desktop::InputArgs& args, desktop::IDisplay& display);
   ~Application();

   Application(const Application& other) = delete;
   Application& operator=(const Application& other) = delete;
   Application(Application&& other) noexcept = delete;
   Application& operator=(Application&& other) noexcept = delete;

   void complete_stage();
   void intitialize();
   void tick() const;
   [[nodiscard]] bool is_init_complete() const;
   [[nodiscard]] desktop::IDisplay& display() const;
   [[nodiscard]] font::FontManger& font_manager();
   [[nodiscard]] graphics_api::Instance& gfx_instance() const;
   [[nodiscard]] graphics_api::Device& gfx_device() const;
   [[nodiscard]] resource::ResourceManager& resource_manager() const;
   [[nodiscard]] render_core::GlyphCache& glyph_cache() const;

 private:
   desktop::IDisplay& m_display;
   LaunchStage m_current_stage{LaunchStage::Uninitialized};
   std::unique_ptr<IStage> m_stage;

   font::FontManger m_font_manager;
   std::unique_ptr<graphics_api::Instance> m_gfx_instance;
   std::unique_ptr<graphics_api::Device> m_gfx_device;
   std::unique_ptr<resource::ResourceManager> m_resource_manager;
   std::unique_ptr<render_core::GlyphCache> m_glyph_cache;
};

}// namespace triglav::launcher
