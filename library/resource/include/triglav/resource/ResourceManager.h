#pragma once

#include <map>
#include <memory>

#include "triglav/font/FontManager.h"
#include "triglav/Name.hpp"

#include "Container.hpp"
#include "Loader.hpp"

namespace triglav::graphics_api {
class Device;
}

namespace triglav::resource {

class ResourceManager
{
 public:
   explicit ResourceManager(graphics_api::Device &device, font::FontManger &fontManager);

   void load_asset_list(std::string_view path);

   void load_asset(Name assetName, std::string_view path);
   [[nodiscard]] bool is_name_registered(Name assetName) const;

   template<ResourceType CResourceType>
   auto &get(const TypedName<CResourceType> name)
   {
      return container<CResourceType>().get(name);
   }

   template<ResourceType CResourceType>
   void load_resource(TypedName<CResourceType> name, std::string_view path)
   {
      if constexpr (Loader<CResourceType>::type == ResourceLoadType::Graphics) {
         container<CResourceType>().register_resource(name, Loader<CResourceType>::load_gpu(m_device, path));
      } else if constexpr (Loader<CResourceType>::type == ResourceLoadType::Font) {
         container<CResourceType>().register_resource(name,
                                                      Loader<CResourceType>::load_font(m_fontManager, path));
      } else if constexpr (Loader<CResourceType>::type == ResourceLoadType::Static) {
         container<CResourceType>().register_resource(name, Loader<CResourceType>::load(path));
      }
   }

   template<ResourceType CResourceType, typename ...TArgs>
   void emplace_resource(TypedName<CResourceType> name, TArgs&& ...args)
   {
      container<CResourceType>().register_emplace(name, std::forward<TArgs>(args)...);
   }

 private:
   template<ResourceType CResourceType>
   Container<CResourceType> &container()
   {
      return *static_cast<Container<CResourceType> *>(m_containers.at(CResourceType).get());
   }

   std::map<ResourceType, std::unique_ptr<IContainer>> m_containers;
   std::map<Name, std::string> m_registeredNames;
   graphics_api::Device &m_device;
   font::FontManger &m_fontManager;
};

}// namespace triglav::resource