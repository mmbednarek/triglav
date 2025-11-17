#pragma once

#include "triglav/Name.hpp"

#include <variant>

namespace c4::yml {

class ConstNodeRef;
class NodeRef;

}// namespace c4::yml

namespace triglav::render_objects {

enum class MaterialTemplate
{
   Basic,
   NormalMap,
   FullPBR,
};

struct MTProperties_Basic
{
   TextureName albedo;
   float roughness;
   float metallic;

   void deserialize_yaml(const c4::yml::ConstNodeRef& node);
   void serialize_yaml(c4::yml::NodeRef& node) const;
};

struct MTProperties_NormalMap
{
   TextureName albedo;
   TextureName normal;
   float roughness;
   float metallic;

   void deserialize_yaml(const c4::yml::ConstNodeRef& node);
   void serialize_yaml(c4::yml::NodeRef& node) const;
};

struct MTProperties_FullPBR
{
   TextureName texture;
   TextureName normal;
   TextureName roughness;
   TextureName metallic;

   void deserialize_yaml(const c4::yml::ConstNodeRef& node);
   void serialize_yaml(c4::yml::NodeRef& node) const;
};

using MaterialProperties = std::variant<MTProperties_Basic, MTProperties_NormalMap, MTProperties_FullPBR>;

struct Material
{
   MaterialTemplate material_template;
   MaterialProperties properties;

   void deserialize_yaml(const c4::yml::ConstNodeRef& node);
   void serialize_yaml(c4::yml::NodeRef& node) const;
};

}// namespace triglav::render_objects