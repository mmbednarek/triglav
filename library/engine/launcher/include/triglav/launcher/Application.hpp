#pragma once
#include "Launcher.hpp"

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
   LaunchStage m_currentStage{LaunchStage::Uninitialized};
   std::unique_ptr<IStage> m_stage;

   font::FontManger m_fontManager;
   std::unique_ptr<graphics_api::Instance> m_gfxInstance;
   std::unique_ptr<graphics_api::Device> m_gfxDevice;
   std::unique_ptr<resource::ResourceManager> m_resourceManager;
   std::unique_ptr<render_core::GlyphCache> m_glyphCache;
};

}// namespace triglav::launcher
