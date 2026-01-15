#pragma once

#include "Name.hpp"
#include "String.hpp"

#include <map>
#include <mutex>

namespace triglav {

class ResourcePathMap
{
 public:
   [[nodiscard]] StringView resolve(ResourceName name) const;
   ResourceName store_path(StringView name);

   static ResourcePathMap& the();

 private:
   std::map<ResourceName, String> m_map;
   mutable std::mutex m_mtx;
};

ResourceName name_from_path(StringView name);
ResourceName resource_sub_path(ResourceName parent, StringView child);

}// namespace triglav