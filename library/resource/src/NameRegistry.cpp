#include "NameRegistry.hpp"

namespace triglav::resource {

void NameRegistry::register_resource(ResourceName resourceName, const std::string_view nameStr)
{
   auto RW_registeredResourceNames = m_registeredResourceNames.access();
   RW_registeredResourceNames->emplace(resourceName, std::string{nameStr});
}
std::optional<std::string> NameRegistry::lookup_resource_name(ResourceName resourceName) const
{
   auto RW_registeredResourceNames = m_registeredResourceNames.read_access();

   auto it = RW_registeredResourceNames->find(resourceName);
   if (it == RW_registeredResourceNames->end()) {
      return std::nullopt;
   }

   return it->second;
}

}// namespace triglav::resource