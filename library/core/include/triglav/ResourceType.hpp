#pragma once

#include <string_view>

#include "TypeMacroList.hpp"

namespace triglav {

enum class ResourceType
{
#define TG_RESOURCE_TYPE(name, ext, cppType) name,
   TG_RESOURCE_TYPE_LIST
#undef TG_RESOURCE_TYPE
           Unknown,
};

template<ResourceType CResourceType>
struct EnumToCppResourceType
{
   // using ResourceType = ...;
};

constexpr ResourceType type_by_extension(const std::string_view extension)
{
#define TG_RESOURCE_TYPE(name, ext, cppType) \
   if (extension == ext)                     \
      return ResourceType::name;

   TG_RESOURCE_TYPE_LIST

#undef TG_RESOURCE_TYPE

   return ResourceType::Unknown;
}

#define TG_RESOURCE_TYPE(name, ext, cppType)                   \
   template<>                                                  \
   struct EnumToCppResourceType<::triglav::ResourceType::name> \
   {                                                           \
      using ResourceType = cppType;                            \
   };

TG_RESOURCE_TYPE_LIST

#undef TG_RESOURCE_TYPE

}// namespace triglav
