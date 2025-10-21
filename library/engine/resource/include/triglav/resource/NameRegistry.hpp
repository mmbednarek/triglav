#pragma once

#include "triglav/Name.hpp"
#include "triglav/threading/SafeAccess.hpp"

#include <map>
#include <optional>
#include <ranges>
#include <string>

namespace triglav::resource {

class NameRegistry
{
 public:
   using ResourceNameMap = std::map<ResourceName, std::string>;

   void register_resource(ResourceName resourceName, std::string_view nameStr);
   std::optional<std::string> lookup_resource_name(ResourceName resourceName) const;

   template<typename T>
   void iterate_names(T t) const
   {
      auto map = m_registeredResourceNames.const_access();
      for (const auto& val : std::views::values(*map)) {
         t(val);
      }
   }

 private:
   threading::SafeReadWriteAccess<std::map<ResourceName, std::string>> m_registeredResourceNames;
};

}// namespace triglav::resource