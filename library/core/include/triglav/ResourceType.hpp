#pragma once

#include "Int.hpp"
#include "TypeMacroList.hpp"

#include <array>
#include <string_view>

namespace triglav {

enum class ResourceType : u32
{
#define TG_RESOURCE_TYPE(name, ext, cppType, stage) name,
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
#define TG_RESOURCE_TYPE(name, ext, cppType, stage) \
   if (extension == ext)                            \
      return ResourceType::name;

   TG_RESOURCE_TYPE_LIST

#undef TG_RESOURCE_TYPE

   return ResourceType::Unknown;
}

#define TG_RESOURCE_TYPE(name, ext, cppType, stage)            \
   template<>                                                  \
   struct EnumToCppResourceType<::triglav::ResourceType::name> \
   {                                                           \
      using ResourceType = cppType;                            \
   };

TG_RESOURCE_TYPE_LIST

#undef TG_RESOURCE_TYPE

constexpr std::array g_resourceStage{
#define TG_RESOURCE_TYPE(name, ext, cppType, stage) stage,
   TG_RESOURCE_TYPE_LIST
#undef TG_RESOURCE_TYPE
};

constexpr int loading_stage_count()
{
   int topStage{};
#define TG_RESOURCE_TYPE(name, ext, cppType, stage) \
   if (topStage < stage)                            \
      topStage = stage;

   TG_RESOURCE_TYPE_LIST

#undef TG_RESOURCE_TYPE

   return topStage + 1;
}

}// namespace triglav
