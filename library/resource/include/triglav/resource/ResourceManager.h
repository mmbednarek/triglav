#pragma once

#include "Container.hpp"
#include "Loader.hpp"
#include "Resource.hpp"

#include "triglav/Name.hpp"
#include "triglav/font/FontManager.h"
#include "triglav/io/Path.h"

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
         container<CResourceType>().register_resource(name, Loader<CResourceType>::load_gpu(m_device, path, props));
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

 private:
   template<ResourceType CResourceType>
   Container<CResourceType>& container()
   {
      return *static_cast<Container<CResourceType>*>(m_containers.at(CResourceType).get());
   }

   std::map<ResourceType, std::unique_ptr<IContainer>> m_containers;
   std::map<ResourceName, std::string> m_registeredNames;
   graphics_api::Device& m_device;
   font::FontManger& m_fontManager;
};

}// namespace triglav::resource