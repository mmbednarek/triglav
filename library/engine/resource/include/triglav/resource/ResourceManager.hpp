#pragma once

#include "Container.hpp"
#include "LevelLoader.hpp"
#include "Loader.hpp"
#include "MaterialLoader.hpp"
#include "MeshLoader.hpp"
#include "NameRegistry.hpp"
#include "Resource.hpp"

#include "LoadContext.hpp"
#include "PathManager.hpp"
#include "triglav/Logging.hpp"
#include "triglav/Name.hpp"
#include "triglav/ResourcePathMap.hpp"
#include "triglav/event/Delegate.hpp"
#include "triglav/font/FontManager.hpp"
#include "triglav/io/Path.hpp"

#include <map>
#include <memory>
#include <string>

namespace triglav::graphics_api {
class Device;
}

namespace triglav::resource {

class ResourceManager
{
   TG_DEFINE_LOG_CATEGORY(ResourceManager)
 public:
   TG_EVENT(OnStartedLoadingAsset, ResourceName)
   TG_EVENT(OnFinishedLoadingAsset, ResourceName, u32, u32)
   TG_EVENT(OnLoadedAssets)

   explicit ResourceManager(graphics_api::Device& device, font::FontManger& font_manager);

   void load_asset_list(const io::Path& path);

   void load_asset(ResourceName asset_name, const io::Path& path);
   [[nodiscard]] bool is_name_registered(ResourceName asset_name) const;

   template<ResourceType CResourceType>
   auto& get(const TypedName<CResourceType> name)
   {
      return container<CResourceType>().get(name);
   }

   template<ResourceType CResourceType>
   void load_resource(TypedName<CResourceType> name, const io::Path& path)
   {
      if constexpr (Loader<CResourceType>::type == ResourceLoadType::Graphics) {
         container<CResourceType>().register_resource(name, Loader<CResourceType>::load_gpu(m_device, name, path));
      } else if constexpr (Loader<CResourceType>::type == ResourceLoadType::GraphicsDependent) {
         container<CResourceType>().register_resource(name, Loader<CResourceType>::load_gpu(*this, m_device, path));
      } else if constexpr (Loader<CResourceType>::type == ResourceLoadType::Font) {
         container<CResourceType>().register_resource(name, Loader<CResourceType>::load_font(m_font_manager, path));
      } else if constexpr (Loader<CResourceType>::type == ResourceLoadType::StaticDependent) {
         container<CResourceType>().register_resource(name, Loader<CResourceType>::load(*this, path));
      } else if constexpr (Loader<CResourceType>::type == ResourceLoadType::Static) {
         container<CResourceType>().register_resource(name, Loader<CResourceType>::load(path));
      }
   }

   template<ResourceType CResourceType, typename... TArgs>
   void emplace_resource(TypedName<CResourceType> name, TArgs&&... args)
   {
      container<CResourceType>().register_emplace(name, std::forward<TArgs>(args)...);
   }

   template<ResourceType CResourceType, typename TFunc>
   void iterate_resources(TFunc func)
   {
      container<CResourceType>().iterate_resources(func);
   }

   void on_finished_loading_resource(ResourceName resource_name, bool skipped = false);

   std::optional<std::string> lookup_name(ResourceName resource_name) const;

   const NameRegistry& name_registry() const;

 private:
   void load_next_stage();

   template<ResourceType CResourceType>
   Container<CResourceType>& container()
   {
      return *static_cast<Container<CResourceType>*>(m_containers.at(CResourceType).get());
   }

   std::unique_ptr<LoadContext> m_load_context{};
   std::map<ResourceType, std::unique_ptr<IContainer>> m_containers;
   NameRegistry m_name_registry;
   graphics_api::Device& m_device;
   font::FontManger& m_font_manager;
};

void resolve_dependencies(std::set<ResourceName>& resource_list);

}// namespace triglav::resource