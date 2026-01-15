#include "ResourcePathMap.hpp"

#include "Format.hpp"
#include "NameResolution.hpp"

#include <cassert>

namespace triglav {

StringView ResourcePathMap::resolve(const ResourceName name) const
{
   std::unique_lock lk{m_mtx};

   const auto it = m_map.find(name);
   if (it == m_map.end()) {
      return StringView{resolve_name(name.name())};
   }
   return it->second.view();
}

ResourceName ResourcePathMap::store_path(const StringView name)
{
   std::unique_lock lk{m_mtx};

   const auto rc_name = make_rc_name(name.to_std());
   if (m_map.contains(rc_name)) {
      assert(m_map.at(rc_name) == name);
      return rc_name;
   }
   m_map.emplace(rc_name, String{name});
   return rc_name;
}

ResourcePathMap& ResourcePathMap::the()
{
   static ResourcePathMap instance;
   return instance;
}

ResourceName name_from_path(const StringView name)
{
   return ResourcePathMap::the().store_path(name);
}

ResourceName resource_sub_path(const ResourceName parent, const StringView child)
{
   const auto resolved = ResourcePathMap::the().resolve(parent);
   const auto sub_path = format("{}/{}", resolved, child);
   return name_from_path(sub_path.view());
}

}// namespace triglav