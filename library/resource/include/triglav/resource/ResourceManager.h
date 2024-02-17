#pragma once

#include <map>
#include <memory>

#include "triglav/Name.hpp"

#include "Container.hpp"
#include "Loader.hpp"

namespace graphics_api {
class Device;
}

namespace triglav::resource {

class ResourceManager
{
 public:
   explicit ResourceManager();

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
      if constexpr (Loader<CResourceType>::is_gpu_resource) {
         container<CResourceType>().register_resource(name, Loader<CResourceType>::load_gpu(m_device, path));
      } else {
         container<CResourceType>().register_resource(name, Loader<CResourceType>::load(path));
      }
   }

 private:
   template<ResourceType CResourceType>
   Container<CResourceType> &container()
   {
      return *static_cast<Container<CResourceType> *>(m_containers.at(CResourceType).get());
   }

   std::map<ResourceType, std::unique_ptr<IContainer>> m_containers;
   graphics_api::Device &m_device;
};
}// namespace triglav::resource