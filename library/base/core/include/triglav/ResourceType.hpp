#pragma once

#include "Int.hpp"
#include "TypeMacroList.hpp"

#include <array>
#include <string_view>

namespace triglav {

enum class ResourceType : u32
{
#define TG_RESOURCE_TYPE(name, ext, cpp_type, stage) name,
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
#define TG_RESOURCE_TYPE(name, ext, cpp_type, stage) \
   if (extension == ext)                             \
      return ResourceType::name;

   TG_RESOURCE_TYPE_LIST

#undef TG_RESOURCE_TYPE

   return ResourceType::Unknown;
}

#define TG_RESOURCE_TYPE(name, ext, cpp_type, stage)           \
   template<>                                                  \
   struct EnumToCppResourceType<::triglav::ResourceType::name> \
   {                                                           \
      using ResourceType = cpp_type;                           \
   };

TG_RESOURCE_TYPE_LIST

#undef TG_RESOURCE_TYPE

constexpr std::array g_resource_stage{
#define TG_RESOURCE_TYPE(name, ext, cpp_type, stage) stage,
   TG_RESOURCE_TYPE_LIST
#undef TG_RESOURCE_TYPE
};

constexpr int loading_stage_count()
{
   int top_stage{};
#define TG_RESOURCE_TYPE(name, ext, cpp_type, stage) \
   if (top_stage < stage)                            \
      top_stage = stage;

   TG_RESOURCE_TYPE_LIST

#undef TG_RESOURCE_TYPE

   return top_stage + 1;
}

constexpr std::array g_resource_stage_extensions{
#define TG_RESOURCE_TYPE(name, ext, cpp_type, stage) std::string_view{ext},

   TG_RESOURCE_TYPE_LIST

#undef TG_RESOURCE_TYPE
};

}// namespace triglav
