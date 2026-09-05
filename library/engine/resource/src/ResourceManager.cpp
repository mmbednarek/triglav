#include "ResourceManager.hpp"

#include "AnimationLoader.hpp"
#include "ArmatureLoader.hpp"
#include "LevelLoader.hpp"
#include "MaterialLoader.hpp"
#include "MeshLoader.hpp"
#include "ShaderLoader.hpp"
#include "TextureLoader.hpp"
#include "TypefaceLoader.hpp"

#include "triglav/TypeMacroList.hpp"
#include "triglav/asset/Asset.hpp"
#include "triglav/project/PathManager.hpp"
#include "triglav/threading/ThreadPool.hpp"

#include <ryml.hpp>

#include <map>
#include <string>

namespace triglav::resource {

using namespace name_literals;

ResourceManager::ResourceManager(graphics_api::Device& device, font::FontManger& font_manager) :
    m_device(device),
    m_font_manager(font_manager)
{
#define TG_RESOURCE_TYPE(name, extension, cpp_type, stage) \
   m_containers.emplace(ResourceType::name, std::make_unique<Container<ResourceType::name>>());
   TG_RESOURCE_TYPE_LIST
#undef TG_RESOURCE_TYPE
}

LoadIndex ResourceManager::load_asset_list(const io::Path& path)
{
   if (m_load_context != nullptr) {
      log_error("Loading assets already in progress");
      assert(false && "ALREADY LOADING ASSETS!");
      return ERROR_LOADING_ASSET;
   }

   const LoadIndex load_index = m_load_id++;
   m_load_context = LoadContext::from_asset_list(path, load_index);
   if (m_load_context == nullptr) {
      log_error("Failed to open asset file list");
      assert(false);
      return ERROR_LOADING_ASSET;
   }

   log_info("Loading {} assets", m_load_context->total_assets());
   this->load_next_stage();
   return load_index;
}

LoadIndex ResourceManager::load_asset(const ResourceName resource_name)
{
   if (m_load_context != nullptr) {
      log_error("Loading assets already in progress");
      assert(false && "ALREADY LOADING ASSETS!");
      return ERROR_LOADING_ASSET;
   }

   const LoadIndex load_index = m_load_id++;
   m_load_context = LoadContext::from_target_asset(resource_name, load_index);
   if (m_load_context == nullptr) {
      log_error("Failed to create load context from asset");
      assert(false);
      return ERROR_LOADING_ASSET;
   }

   log_info("Loading {} assets", m_load_context->total_assets());
   this->load_next_stage();

   return load_index;
}

LoadIndex ResourceManager::load_assets(const std::span<ResourceName> resource_name)
{
   if (m_load_context != nullptr) {
      log_error("Loading assets already in progress");
      assert(false && "ALREADY LOADING ASSETS!");
      return ERROR_LOADING_ASSET;
   }

   const LoadIndex load_index = m_load_id++;
   m_load_context = LoadContext::from_assets(resource_name, load_index);
   if (m_load_context == nullptr) {
      log_error("Failed to create load context from asset");
      assert(false);
      return ERROR_LOADING_ASSET;
   }

   log_info("Loading {} assets", m_load_context->total_assets());
   this->load_next_stage();

   return load_index;
}

void ResourceManager::load_next_stage()
{
   if (m_load_context == nullptr) {
      log_error("Cannot load stage: no asset loading in progress");
      assert(false);
      return;
   }

   auto& stage = m_load_context->next_stage();

   log_info("Loading {} assets in stage {}", stage.resource_list.size(), m_load_context->current_stage_id());

   for (const auto& [rc_name, rc_path] : stage.resource_list) {
      if (not rc_path.exists()) {
         log_error("failed to load resource: {}, file not found", rc_path.string());
         flush_logs();
         assert(false);
      }

      m_name_registry.register_resource(rc_name, rc_path.string());
      threading::ThreadPool::the().issue_job([&] { this->load_asset_internal(rc_name, rc_path); });
   }

   flush_logs();
}

void ResourceManager::load_asset_internal(const ResourceName asset_name, const io::Path& path)
{
   log_info("[THREAD: {}] Loading asset {}", threading::this_thread_id(),
            m_name_registry.lookup_resource_name(asset_name).value_or("UNKNOWN"));

   this->event_OnStartedLoadingAsset.publish(asset_name);

   if (this->is_name_registered(asset_name)) {
      this->on_finished_loading_resource(asset_name);
      return;
   }

   switch (asset_name.type()) {
#define TG_RESOURCE_TYPE(name, extension, cpp_type, stage)       \
   case ResourceType::name:                                      \
      this->load_resource<ResourceType::name>(asset_name, path); \
      break;
      TG_RESOURCE_TYPE_LIST_LOADABLE
#undef TG_RESOURCE_TYPE
   case ResourceType::Unknown:
   default:
      break;
   }

   this->on_finished_loading_resource(asset_name);
}

bool ResourceManager::is_name_registered(const ResourceName asset_name) const
{
   if (not m_containers.contains(asset_name.type()))
      return false;

   auto& container = m_containers.at(asset_name.type());
   return container->is_name_registered(asset_name);
}

void ResourceManager::on_finished_loading_resource(ResourceName resource_name, const bool skipped)
{
   if (m_load_context == nullptr) {
      log_error("Cannot load stage: no asset loading in progress");
      return;
   }

   if (skipped == false) {
      log_info("[THREAD: {}] [{}/{}] Successfully loaded {}", threading::this_thread_id(), m_load_context->total_loaded_assets(),
               m_load_context->total_assets(), m_name_registry.lookup_resource_name(resource_name).value_or("UNKNOWN"));
      this->event_OnFinishedLoadingAsset.publish(resource_name, m_load_context->total_loaded_assets(), m_load_context->total_assets());
   }

   flush_logs();

   const auto result = m_load_context->finish_loading_asset();

   switch (result) {
   case FinishLoadingAssetResult::FinishedStage:
      log_info("Loading stage {} DONE", m_load_context->current_stage_id());
      this->load_next_stage();
      break;
   case FinishLoadingAssetResult::FinishedLoadingAssets: {
      log_info("Loading assets DONE");
      const auto load_index = m_load_context->load_index();
      m_load_context.reset();
      this->event_OnLoadedAssets.publish(load_index);
      break;
   }
   case FinishLoadingAssetResult::None:
      break;
   }
}

std::optional<std::string> ResourceManager::lookup_name(const ResourceName resource_name) const
{
   return m_name_registry.lookup_resource_name(resource_name);
}

const NameRegistry& ResourceManager::name_registry() const
{
   return m_name_registry;
}

void resolve_dependencies(std::set<ResourceName>& resource_list)
{
   std::set<ResourceName> child_deps;
   for (const auto rc : resource_list) {
      const auto path = project::PathManager::the().translate_path(rc);

      rc.match([&]<typename TName>(TName /*typed_rc*/) {
         if constexpr (CollectsDependencies<Loader<TName::resource_type>>) {
            Loader<TName::resource_type>::collect_dependencies(child_deps, path);
         }
      });
   }

   if (!child_deps.empty()) {
      resolve_dependencies(child_deps);
      resource_list.insert_range(child_deps);
   }
}

}// namespace triglav::resource