#include "ResourceManager.h"

#include "LevelLoader.h"
#include "MaterialLoader.h"
#include "ModelLoader.h"
#include "PathManager.h"
#include "ShaderLoader.h"
#include "TextureLoader.h"
#include "TypefaceLoader.h"

#include "triglav/TypeMacroList.hpp"
#include "triglav/io/File.h"
#include "triglav/threading/ThreadPool.h"

#include <ryml.hpp>
#include <spdlog/spdlog.h>

#include <map>
#include <string>

namespace triglav::resource {

using namespace name_literals;

namespace {

std::vector<ResourceStage> parse_asset_list(const io::Path& path)
{
   std::vector<ResourceStage> result{};
   result.resize(loading_stage_count());

   auto file = io::read_whole_file(path);
   auto tree =
      ryml::parse_in_place(c4::substr{const_cast<char*>(path.string().data()), path.string().size()}, c4::substr{file.data(), file.size()});
   auto resources = tree["resources"];

   for (const auto node : resources) {
      auto name = node["name"].val();
      auto source = node["source"].val();

      ResourceProperties properties;

      if (node.has_child("properties")) {
         auto propertiesNode = node["properties"];
         for (const auto property : propertiesNode) {
            auto key = property.key();
            auto value = property.val();
            properties.add(make_name_id({key.data(), key.size()}), std::string{value.data(), value.size()});
         }
      }

      auto resourceName = make_rc_name(std::string{name.data(), name.size()});
      result[resourceName.loading_stage()].resourceList.emplace_back(std::string{name.data(), name.size()},
                                                                     std::string{source.data(), source.size()}, std::move(properties));
   }

   return result;
}

}// namespace

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
      spdlog::error("Loading assets already in progress");
      return;
   }
   m_loadContext = LoadContext::from_asset_list(path);

   spdlog::info("Loading {} assets", m_loadContext->total_assets());
   this->load_next_stage();
}

void ResourceManager::load_next_stage()
{
   if (m_loadContext == nullptr) {
      spdlog::error("Cannot load stage: no asset loading in progress");
      return;
   }

   auto& stage = m_loadContext->next_stage();

   spdlog::info("Loading {} assets in stage {}", stage.resourceList.size(), m_loadContext->current_stage_id());

   auto buildPath = PathManager::the().build_path();
   auto contentPath = PathManager::the().content_path();

   for (const auto& [nameStr, source, props] : stage.resourceList) {
      if (props.get_bool("rt_only"_name) && !(m_device.enabled_features() & graphics_api::DeviceFeature::RayTracing)) {
         spdlog::info("Skipped asset {}, ray tracing is disabled", nameStr);
         this->on_finished_loading_resource(make_rc_name(nameStr), true);
         continue;
      }

      auto resourcePath = buildPath.sub(source);
      if (not resourcePath.exists()) {
         resourcePath = contentPath.sub(source);

         if (not resourcePath.exists()) {
            spdlog::error("failed to load resource: {}, file not found", source);
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
   spdlog::info("[THREAD: {}] Loading asset {}", threading::this_thread_id(),
                m_nameRegistry.lookup_resource_name(assetName).value_or("UNKNOWN"));

   this->OnStartedLoadingAsset.publish(assetName);

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
      spdlog::error("Cannot load stage: no asset loading in progress");
      return;
   }

   if (skipped == false) {
      spdlog::info("[THREAD: {}] [{}/{}] Successfully loaded {}", threading::this_thread_id(), m_loadContext->total_loaded_assets(),
                   m_loadContext->total_assets(), m_nameRegistry.lookup_resource_name(resourceName).value_or("UNKNOWN"));
      this->OnFinishedLoadingAsset.publish(resourceName, m_loadContext->total_loaded_assets(), m_loadContext->total_assets());
   }

   const auto result = m_loadContext->finish_loading_asset();

   switch (result) {
   case FinishLoadingAssetResult::FinishedStage:
      spdlog::info("Loading stage {} DONE", m_loadContext->current_stage_id());
      this->load_next_stage();
      break;
   case FinishLoadingAssetResult::FinishedLoadingAssets:
      spdlog::info("Loading assets DONE");
      m_loadContext.reset();
      this->OnLoadedAssets.publish();
      break;
   case FinishLoadingAssetResult::None:
      break;
   }
}

std::optional<std::string> ResourceManager::lookup_name(const ResourceName resourceName) const
{
   return m_nameRegistry.lookup_resource_name(resourceName);
}

}// namespace triglav::resource