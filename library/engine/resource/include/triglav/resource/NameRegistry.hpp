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

   void register_resource(ResourceName resource_name, std::string_view name_str);
   std::optional<std::string> lookup_resource_name(ResourceName resource_name) const;

   template<typename T>
   void iterate_names(T t) const
   {
      auto map = m_registered_resource_names.const_access();
      for (const auto& val : std::views::values(*map)) {
         t(val);
      }
   }

 private:
   threading::SafeReadWriteAccess<std::map<ResourceName, std::string>> m_registered_resource_names;
};

}// namespace triglav::resource