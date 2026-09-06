#include "Engine.hpp"

#include "triglav/io/File.hpp"
#include "triglav/project/PathManager.hpp"
#include "triglav/project/ProjectManager.hpp"

#include <ryml.hpp>

namespace triglav::engine {

using namespace name_literals;

void Engine::initialize(graphics_api::Device& device)
{
   m_status.store(EngineStatus::Initializing);
   m_resource_manager = std::make_unique<resource::ResourceManager>(device, m_font_manger);
   TG_CONNECT_OPT(*m_resource_manager, OnLoadedAssets, on_loaded_assets);
   log_info("Initialising engine");

   project::PathManager::the().discover_resources();

   m_load_index = m_resource_manager->load_asset_list(project::PathManager::the().translate_path("engine/index.yaml"_rc));
   assert(m_load_index != resource::ERROR_LOADING_ASSET);
}

void Engine::destroy()
{
   sink_OnLoadedAssets.release();
   m_levels.clear();
   m_pending_level.reset();
   m_resource_manager.reset();
   m_status.store(EngineStatus::Uninitialized);
}

void Engine::load_level(const LevelName level_name)
{
   assert(m_resource_manager != nullptr);

   log_info("Loading level {}", ResourcePathMap::the().resolve(level_name));

   const auto level_path = project::PathManager::the().translate_path(level_name);
   auto file = io::open_file(level_path, io::FileMode::Read);
   assert(file.has_value());

   if (project::this_project() != "triglav_editor"_name) {
      // If not the editor, reset all levels
      m_levels.clear();
   }
   m_pending_level_name = level_name;
   m_pending_level = std::make_unique<world::Level>();
   assert(m_pending_level->deserialize(**file));

   std::vector<ResourceName> resource_list;

   for (const auto [entity_id, mesh] : m_pending_level->all<world::Mesh>()) {
      resource_list.emplace_back(mesh.name);
   }
   for (const auto [entity_id, arm] : m_pending_level->all<world::Armature>()) {
      resource_list.emplace_back(arm.name);
   }

   m_status.store(EngineStatus::LoadingLevel);
   m_load_index = m_resource_manager->load_assets(resource_list);
   assert(m_load_index != resource::ERROR_LOADING_ASSET);
}

void Engine::on_loaded_assets(const resource::LoadIndex load_index)
{
   if (m_load_index != load_index)
      return;
   m_load_index = resource::ERROR_LOADING_ASSET;

   const auto status = m_status.load();
   switch (status) {
   case EngineStatus::Initializing: {
      m_status.store(EngineStatus::Ready);
      event_OnEngineReady.publish();
      return;
   }
   case EngineStatus::LoadingLevel: {
      log_info("Level ready");
      m_status.store(EngineStatus::LevelLoaded);
      m_levels.emplace(m_pending_level_name, std::move(m_pending_level));
      m_current_level_name = m_pending_level_name;
      m_current_level = m_levels.at(m_current_level_name).get();
      m_pending_level_name = {};
      event_OnLevelLoaded.publish();
      return;
   }
   default:
      assert(false);
      break;
   }
}

resource::ResourceManager& Engine::resource_manager() const
{
   assert(m_resource_manager != nullptr);
   return *m_resource_manager;
}

void Engine::on_begin_frame(float /*delta_time*/) const
{
   if (m_current_level != nullptr) {
      m_current_level->flush();
   }
}

void Engine::unload_level(const LevelName name)
{
   if (m_current_level_name == name) {
      m_current_level_name = {};
      m_current_level = nullptr;
   }
   m_levels.erase(name);
}

void Engine::set_active_level(const LevelName name)
{
   m_current_level_name = name;
   m_current_level = m_levels.at(name).get();
}

world::Level* Engine::current_level() const
{
   assert(m_current_level == nullptr || m_current_level == m_levels.at(m_current_level_name).get());
   return m_current_level;
}

world::Level* Engine::level_by_name(const LevelName name) const
{
   const auto it = m_levels.find(name);
   if (it == m_levels.end()) {
      return nullptr;
   }
   return it->second.get();
}

Engine& Engine::the()
{
   static Engine engine;
   return engine;
}

}// namespace triglav::engine