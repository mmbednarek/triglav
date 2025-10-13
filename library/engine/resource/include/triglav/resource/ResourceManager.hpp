#pragma once

#include "Container.hpp"
#include "Loader.hpp"
#include "NameRegistry.hpp"
#include "Resource.hpp"

#include "LoadContext.hpp"
#include "triglav/Name.hpp"
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
 public:
   TG_EVENT(OnStartedLoadingAsset, ResourceName)
   TG_EVENT(OnFinishedLoadingAsset, ResourceName, u32, u32)
   TG_EVENT(OnLoadedAssets)

   explicit ResourceManager(graphics_api::Device& device, font::FontManger& fontManager);

   void load_asset_list(const io::Path& path);

   void load_asset(ResourceName assetName, const io::Path& path, const ResourceProperties& props);
   [[nodiscard]] bool is_name_registered(ResourceName assetName) const;

   template<ResourceType CResourceType>
   auto& get(const TypedName<CResourceType> name)
   {
      return container<CResourceType>().get(name);
   }

   template<ResourceType CResourceType>
   void load_resource(TypedName<CResourceType> name, const io::Path& path, const ResourceProperties& props)
   {
      if constexpr (Loader<CResourceType>::type == ResourceLoadType::Graphics) {
         container<CResourceType>().register_resource(name, Loader<CResourceType>::load_gpu(m_device, name, path, props));
      } else if constexpr (Loader<CResourceType>::type == ResourceLoadType::GraphicsDependent) {
         container<CResourceType>().register_resource(name, Loader<CResourceType>::load_gpu(*this, m_device, path));
      } else if constexpr (Loader<CResourceType>::type == ResourceLoadType::Font) {
         container<CResourceType>().register_resource(name, Loader<CResourceType>::load_font(m_fontManager, path));
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

   void on_finished_loading_resource(ResourceName resourceName, bool skipped = false);

   std::optional<std::string> lookup_name(ResourceName resourceName) const;

 private:
   void load_next_stage();

   template<ResourceType CResourceType>
   Container<CResourceType>& container()
   {
      return *static_cast<Container<CResourceType>*>(m_containers.at(CResourceType).get());
   }

   std::unique_ptr<LoadContext> m_loadContext{};
   std::map<ResourceType, std::unique_ptr<IContainer>> m_containers;
   NameRegistry m_nameRegistry;
   graphics_api::Device& m_device;
   font::FontManger& m_fontManager;
};

}// namespace triglav::resource