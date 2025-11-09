#include "ResourceManager.hpp"

#include "LevelLoader.hpp"
#include "MaterialLoader.hpp"
#include "MeshLoader.hpp"
#include "PathManager.hpp"
#include "ShaderLoader.hpp"
#include "TextureLoader.hpp"
#include "TypefaceLoader.hpp"

#include "triglav/TypeMacroList.hpp"
#include "triglav/io/File.hpp"
#include "triglav/threading/ThreadPool.hpp"

#include <ryml.hpp>

#include <map>
#include <string>

namespace triglav::resource {

using namespace name_literals;

ResourceManager::ResourceManager(graphics_api::Device& device, font::FontManger& fontManager) :
    m_device(device),
    m_fontManager(fontManager)
{
#define TG_RESOURCE_TYPE(name, extension, cppType, stage) \
   m_containers.emplace(ResourceType::name, std::make_unique<Container<ResourceType::name>>());
   TG_RESOURCE_TYPE_LIST
#undef TG_RESOURCE_TYPE
}

void ResourceManager::load_asset_list(const io::Path& path)
{
   if (m_loadContext != nullptr) {
      log_error("Loading assets already in progress");
      return;
   }
   m_loadContext = LoadContext::from_asset_list(path);
   if (m_loadContext == nullptr) {
      log_error("Failed to open asset file list");
      return;
   }

   log_info("Loading {} assets", m_loadContext->total_assets());
   this->load_next_stage();
}

void ResourceManager::load_next_stage()
{
   if (m_loadContext == nullptr) {
      log_error("Cannot load stage: no asset loading in progress");
      return;
   }

   auto& stage = m_loadContext->next_stage();

   log_info("Loading {} assets in stage {}", stage.resourceList.size(), m_loadContext->current_stage_id());

   auto buildPath = PathManager::the().build_path();
   auto contentPath = PathManager::the().content_path();

   for (const auto& [nameStr, source, props] : stage.resourceList) {
      if (props.get_bool("rt_only"_name) && !(m_device.enabled_features() & graphics_api::DeviceFeature::RayTracing)) {
         log_info("Skipped asset {}, ray tracing is disabled", nameStr);
         this->on_finished_loading_resource(make_rc_name(nameStr), true);
         continue;
      }

      auto resourcePath = buildPath.sub(source);
      if (not resourcePath.exists()) {
         resourcePath = contentPath.sub(source);

         if (not resourcePath.exists()) {
            log_error("failed to load resource: {}, file not found", source);
            continue;
         }
      }

      auto name = make_rc_name(nameStr);
      m_nameRegistry.register_resource(name, nameStr);
      threading::ThreadPool::the().issue_job([this, name, resourcePath, props] { this->load_asset(name, resourcePath, props); });
   }
}

void ResourceManager::load_asset(const ResourceName assetName, const io::Path& path, const ResourceProperties& props)
{
   log_info("[THREAD: {}] Loading asset {}", threading::this_thread_id(),
            m_nameRegistry.lookup_resource_name(assetName).value_or("UNKNOWN"));

   this->event_OnStartedLoadingAsset.publish(assetName);

   switch (assetName.type()) {
#define TG_RESOURCE_TYPE(name, extension, cppType, stage)              \
   case ResourceType::name:                                            \
      this->load_resource<ResourceType::name>(assetName, path, props); \
      break;
      TG_RESOURCE_TYPE_LIST
#undef TG_RESOURCE_TYPE
   case ResourceType::Unknown:
      break;
   }

   this->on_finished_loading_resource(assetName);
}

bool ResourceManager::is_name_registered(const ResourceName assetName) const
{
   if (not m_containers.contains(assetName.type()))
      return false;

   auto& container = m_containers.at(assetName.type());
   return container->is_name_registered(assetName);
}

void ResourceManager::on_finished_loading_resource(ResourceName resourceName, const bool skipped)
{
   if (m_loadContext == nullptr) {
      log_error("Cannot load stage: no asset loading in progress");
      return;
   }

   if (skipped == false) {
      log_info("[THREAD: {}] [{}/{}] Successfully loaded {}", threading::this_thread_id(), m_loadContext->total_loaded_assets(),
               m_loadContext->total_assets(), m_nameRegistry.lookup_resource_name(resourceName).value_or("UNKNOWN"));
      this->event_OnFinishedLoadingAsset.publish(resourceName, m_loadContext->total_loaded_assets(), m_loadContext->total_assets());
   }

   const auto result = m_loadContext->finish_loading_asset();

   switch (result) {
   case FinishLoadingAssetResult::FinishedStage:
      log_info("Loading stage {} DONE", m_loadContext->current_stage_id());
      this->load_next_stage();
      break;
   case FinishLoadingAssetResult::FinishedLoadingAssets:
      log_info("Loading assets DONE");
      m_loadContext.reset();
      this->event_OnLoadedAssets.publish();
      break;
   case FinishLoadingAssetResult::None:
      break;
   }
}

std::optional<std::string> ResourceManager::lookup_name(const ResourceName resourceName) const
{
   return m_nameRegistry.lookup_resource_name(resourceName);
}

const NameRegistry& ResourceManager::name_registry() const
{
   return m_nameRegistry;
}

}// namespace triglav::resource