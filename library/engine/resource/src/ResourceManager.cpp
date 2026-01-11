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

ResourceManager::ResourceManager(graphics_api::Device& device, font::FontManger& font_manager) :
    m_device(device),
    m_font_manager(font_manager)
{
#define TG_RESOURCE_TYPE(name, extension, cpp_type, stage) \
   m_containers.emplace(ResourceType::name, std::make_unique<Container<ResourceType::name>>());
   TG_RESOURCE_TYPE_LIST
#undef TG_RESOURCE_TYPE
}

void ResourceManager::load_asset_list(const io::Path& path)
{
   if (m_load_context != nullptr) {
      log_error("Loading assets already in progress");
      return;
   }
   m_load_context = LoadContext::from_asset_list(path);
   if (m_load_context == nullptr) {
      log_error("Failed to open asset file list");
      return;
   }

   log_info("Loading {} assets", m_load_context->total_assets());
   this->load_next_stage();
}

void ResourceManager::load_asset(const ResourceName resource_name)
{
   if (m_load_context != nullptr) {
      log_error("Loading assets already in progress");
      return;
   }
   m_load_context = LoadContext::from_target_asset(resource_name);
   if (m_load_context == nullptr) {
      log_error("Failed to create load context from asset");
      return;
   }

   log_info("Loading {} assets", m_load_context->total_assets());
   this->load_next_stage();
}

const std::vector<io::Path>& ResourceManager::source_paths()
{
   if (!m_source_paths.empty())
      return m_source_paths;

   m_source_paths.emplace_back(PathManager::the().content_path());
   m_source_paths.emplace_back(PathManager::the().build_path());

   auto engine_content = PathManager::the().engine_content_path();
   if (engine_content.has_value()) {
      m_source_paths.emplace_back(*engine_content);
   }

   auto project_path = PathManager::the().project_content_path();
   if (project_path.has_value()) {
      m_source_paths.emplace_back(*project_path);
   }

   return m_source_paths;
}

void ResourceManager::load_next_stage()
{
   if (m_load_context == nullptr) {
      log_error("Cannot load stage: no asset loading in progress");
      return;
   }

   auto& stage = m_load_context->next_stage();

   log_info("Loading {} assets in stage {}", stage.resource_list.size(), m_load_context->current_stage_id());

   const auto& src_paths = this->source_paths();

   for (const auto& rc_path : stage.resource_list) {
      bool found = false;
      for (const auto& src_path : src_paths) {
         auto resource_path = src_path.sub(rc_path.to_std_view());
         if (not resource_path.exists())
            continue;

         auto name = name_from_path(rc_path.view());
         m_name_registry.register_resource(name, rc_path.to_std_view());
         threading::ThreadPool::the().issue_job([this, name, resource_path] { this->load_asset_internal(name, resource_path); });

         found = true;
         break;
      }
      if (!found) {
         log_error("failed to load resource: {}, file not found", rc_path);
      }
   }
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
      TG_RESOURCE_TYPE_LIST
#undef TG_RESOURCE_TYPE
   case ResourceType::Unknown:
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

   const auto result = m_load_context->finish_loading_asset();

   switch (result) {
   case FinishLoadingAssetResult::FinishedStage:
      log_info("Loading stage {} DONE", m_load_context->current_stage_id());
      this->load_next_stage();
      break;
   case FinishLoadingAssetResult::FinishedLoadingAssets:
      log_info("Loading assets DONE");
      m_load_context.reset();
      this->event_OnLoadedAssets.publish();
      break;
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
      const auto path_str = ResourcePathMap::the().resolve(rc);
      if (path_str.size() == 0)
         return;
      const auto path = PathManager::the().project_content_path()->sub(path_str.to_std());

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