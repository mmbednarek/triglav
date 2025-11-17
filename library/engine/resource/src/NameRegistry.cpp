#include "NameRegistry.hpp"

namespace triglav::resource {

void NameRegistry::register_resource(ResourceName resource_name, const std::string_view name_str)
{
   auto RW_registered_resource_names = m_registered_resource_names.access();
   RW_registered_resource_names->emplace(resource_name, std::string{name_str});
}

std::optional<std::string> NameRegistry::lookup_resource_name(ResourceName resource_name) const
{
   auto RW_registered_resource_names = m_registered_resource_names.read_access();

   auto it = RW_registered_resource_names->find(resource_name);
   if (it == RW_registered_resource_names->end()) {
      return std::nullopt;
   }

   return it->second;
}

}// namespace triglav::resource