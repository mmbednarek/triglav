#pragma once

#include "triglav/resource/ResourceManager.hpp"
#include "triglav/font/FontManager.hpp"
#include "triglav/Logging.hpp"

#include <atomic>

namespace triglav::graphics_api {
class Device;
}

namespace triglav::world {
class Level;
}

namespace triglav::engine {

enum class EngineStatus
{
   Uninitialized,
   Initializing,
   Ready,
   LoadingLevel,
   LevelLoaded,
};

class Engine
{
   TG_DEFINE_LOG_CATEGORY(Engine)
public:
   TG_EVENT(OnEngineReady)
   TG_EVENT(OnLevelLoaded)

   using Self = Engine;

   void initialize(graphics_api::Device& device);
   void destroy();
   void load_level(LevelName level_name);
   void on_loaded_assets();
   [[nodiscard]] resource::ResourceManager& resource_manager() const;
   void on_begin_frame(float delta_time) const;
   void unload_level(LevelName name);
   void set_active_level(LevelName name);

   [[nodiscard]] world::Level* current_level() const;

   static Engine& the();
private:
   font::FontManger m_font_manger;
   std::unique_ptr<resource::ResourceManager> m_resource_manager;
   LevelName m_current_level_name;
   world::Level* m_current_level = nullptr;
   LevelName m_pending_level_name;
   std::unique_ptr<world::Level> m_pending_level;
   std::map<LevelName, std::unique_ptr<world::Level>> m_levels;
   std::atomic<EngineStatus> m_status = EngineStatus::Uninitialized;

   TG_OPT_SINK(resource::ResourceManager, OnLoadedAssets);
};

}