#pragma once

#include "triglav/Logging.hpp"
#include "triglav/Name.hpp"
#include "triglav/ResourcePathMap.hpp"
#include "triglav/threading/SharedMutex.hpp"

#include <map>
#include <mutex>
#include <ranges>
#include <shared_mutex>
#include <utility>

namespace triglav::resource {

class IContainer
{
 public:
   virtual ~IContainer() = default;

   [[nodiscard]] virtual bool is_name_registered(ResourceName name) const = 0;
};

template<ResourceType CResourceType>
class Container final : public IContainer
{
   TG_DEFINE_LOG_CATEGORY(ResourceContainer)
 public:
   using ResName = TypedName<CResourceType>;
   using ValueType = typename EnumToCppResourceType<CResourceType>::ResourceType;

   ValueType& get(const ResName name)
   {
      std::shared_lock lk{m_mutex};
      if (!m_map.contains(name)) {
         log_error("Resource not found: {}", ResourcePathMap::the().resolve(name));
         flush_logs();
         assert(false);
      }
      return m_map.at(name);
   }

   template<typename... TArgs>
   void register_emplace(const ResName name, TArgs... args)
   {
      std::unique_lock lk{m_mutex};
      m_map.emplace(name, std::forward<TArgs>(args)...);
   }

   void register_resource(const ResName name, ValueType&& resource)
   {
      std::unique_lock lk{m_mutex};
      m_map.emplace(name, std::move(resource));
   }

   [[nodiscard]] bool is_name_registered(const ResourceName name) const override
   {
      if (name.type() != CResourceType)
         return false;

      std::shared_lock lk{m_mutex};
      return m_map.contains(ResName{name});
   }

   template<typename TFunc>
   void iterate_resources(TFunc func)
   {
      for (const auto& [name, value] : m_map) {
         func(name, value);
      }
   }

 private:
   std::map<ResName, ValueType> m_map{};
   mutable threading::SharedMutex m_mutex;
};

}// namespace triglav::resource