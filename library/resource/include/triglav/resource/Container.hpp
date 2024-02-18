#pragma once

#include "triglav/Name.hpp"

#include <map>
#include <utility>

namespace triglav::resource {

class IContainer
{
 public:
   virtual ~IContainer() = default;

   [[nodiscard]] virtual bool is_name_registered(Name name) const = 0;
};

template<ResourceType CResourceType>
class Container final : public IContainer
{
 public:
   using ResName = TypedName<CResourceType>;
   using ValueType = typename EnumToCppResourceType<CResourceType>::ResourceType;

   ValueType &get(const ResName name)
   {
      return m_map.at(name);
   }

   template<typename... TArgs>
   void register_emplace(const ResName name, TArgs... args)
   {
      m_map.emplace(name, std::forward<TArgs>(args)...);
   }

   void register_resource(const ResName name, ValueType &&resource)
   {
      m_map.emplace(name, std::move(resource));
   }

   [[nodiscard]] bool is_name_registered(const Name name) const override
   {
      if (name.type() != CResourceType)
         return false;

      return m_map.contains(ResName{name});
   }

 private:
   std::map<ResName, ValueType> m_map{};
};

}// namespace triglav::resource