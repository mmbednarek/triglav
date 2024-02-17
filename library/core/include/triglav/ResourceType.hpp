#pragma once

#include <string_view>

namespace triglav {

enum class ResourceType
{
   Texture,
   Mesh,
   FragmentShader,
   VertexShader,
   Material,
   Model,
   ResourceTypeFace,
   Unknown
};

template<ResourceType CResourceType>
struct EnumToCppResourceType
{
   // using ResourceType = ...;
};

constexpr ResourceType type_by_extension(const std::string_view extension)
{
   if (extension == "texture")
      return ResourceType::Texture;
   if (extension == "mesh")
      return ResourceType::Mesh;
   if (extension == "fshader")
      return ResourceType::FragmentShader;
   if (extension == "vshader")
      return ResourceType::VertexShader;
   if (extension == "mat")
      return ResourceType::Material;
   if (extension == "model")
      return ResourceType::Model;
   if (extension == "typeface")
      return ResourceType::ResourceTypeFace;

   return ResourceType::Unknown;
}

}// namespace resource

#define DEFINE_RESOURCE_TYPE(enum, type)      \
   namespace triglav {                       \
   template<>                                 \
   struct EnumToCppResourceType<enum>                 \
   {                                          \
      using ResourceType = type;                      \
   };                                         \

namespace graphics_api {
class Texture;
}

DEFINE_RESOURCE_TYPE(triglav::ResourceType::Texture, graphics_api::Texture)