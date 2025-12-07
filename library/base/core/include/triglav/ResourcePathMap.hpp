#pragma once

#include "Name.hpp"
#include "String.hpp"

#include <map>

namespace triglav {

class ResourcePathMap
{
 public:
   [[nodiscard]] StringView resolve(ResourceName name) const;
   ResourceName store_path(StringView name);

   static ResourcePathMap& the();

 private:
   std::map<ResourceName, String> m_map;
};

ResourceName name_from_path(StringView name);

}// namespace triglav