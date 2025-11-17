#include "Material.hpp"

#include <ryml.hpp>
#include <string>

namespace triglav::render_objects {

namespace {

std::string to_string(const MaterialTemplate mat_template)
{
   switch (mat_template) {
   case MaterialTemplate::Basic:
      return "basic";
   case MaterialTemplate::NormalMap:
      return "normal_map";
   case MaterialTemplate::FullPBR:
      return "full_pbr";
   }
   return "";
}

MaterialTemplate material_template_from_string(const ryml::csubstr str)
{
   if (str == "basic") {
      return MaterialTemplate::Basic;
   }
   if (str == "normal_map") {
      return MaterialTemplate::NormalMap;
   }
   if (str == "full_pbr") {
      return MaterialTemplate::FullPBR;
   }
   return {};
}

TextureName to_texture_name(const ryml::csubstr str)
{
   if (std::ranges::all_of(str, isdigit)) {
      return TextureName{std::stoull({str.data(), str.size()})};
   }

   return make_rc_name({str.data(), str.size()});
}

float to_float(const ryml::csubstr str)
{
   return std::stof({str.data(), str.size()});
}

}// namespace

void MTProperties_Basic::deserialize_yaml(const c4::yml::ConstNodeRef& node)
{
   this->albedo = to_texture_name(node["albedo"].val());
   this->metallic = to_float(node["metallic"].val());
   this->roughness = to_float(node["roughness"].val());
}

void MTProperties_Basic::serialize_yaml(c4::yml::NodeRef& node) const
{
   auto tex_str = std::to_string(this->albedo.name());
   node["albedo"] = {tex_str.data(), tex_str.size()};
   auto metallic_str = std::to_string(this->metallic);
   node["metallic"] = {metallic_str.data(), metallic_str.size()};
   auto roughness_str = std::to_string(this->roughness);
   node["roughness"] = {roughness_str.data(), roughness_str.size()};
}

void MTProperties_NormalMap::deserialize_yaml(const c4::yml::ConstNodeRef& node)
{
   this->albedo = to_texture_name(node["albedo"].val());
   this->normal = to_texture_name(node["normal"].val());
   this->metallic = to_float(node["metallic"].val());
   this->roughness = to_float(node["roughness"].val());
}

void MTProperties_NormalMap::serialize_yaml(c4::yml::NodeRef& node) const
{
   auto tex = std::to_string(this->albedo.name());
   node["albedo"] = {tex.data(), tex.size()};
   auto normal_map = std::to_string(this->normal.name());
   node["normal"] = {normal_map.data(), normal_map.size()};
   auto metallic_str = std::to_string(this->metallic);
   node["metallic"] = {metallic_str.data(), metallic_str.size()};
   auto roughness_str = std::to_string(this->roughness);
   node["roughness"] = {roughness_str.data(), roughness_str.size()};
}

void MTProperties_FullPBR::deserialize_yaml(const c4::yml::ConstNodeRef& node)
{
   this->texture = to_texture_name(node["albedo"].val());
   this->normal = to_texture_name(node["normal"].val());
   this->metallic = to_texture_name(node["metallic"].val());
   this->roughness = to_texture_name(node["roughness"].val());
}

void MTProperties_FullPBR::serialize_yaml(c4::yml::NodeRef& node) const
{
   auto tex = std::to_string(this->texture.name());
   node["albedo"] = {tex.data(), tex.size()};
   auto normal_map = std::to_string(this->normal.name());
   node["normal"] = {normal_map.data(), normal_map.size()};
   auto metallic_str = std::to_string(this->metallic.name());
   node["metallic"] = {metallic_str.data(), metallic_str.size()};
   auto roughness_str = std::to_string(this->roughness.name());
   node["roughness"] = {roughness_str.data(), roughness_str.size()};
}

void Material::deserialize_yaml(const c4::yml::ConstNodeRef& node)
{
   this->material_template = material_template_from_string(node["template"].val());
   const auto properties_node = node["properties"];

   switch (this->material_template) {
   case MaterialTemplate::Basic: {
      MTProperties_Basic basic;
      basic.deserialize_yaml(properties_node);
      this->properties = basic;
      break;
   }
   case MaterialTemplate::NormalMap: {
      MTProperties_NormalMap basic;
      basic.deserialize_yaml(properties_node);
      this->properties = basic;
      break;
   }
   case MaterialTemplate::FullPBR: {
      MTProperties_FullPBR basic;
      basic.deserialize_yaml(properties_node);
      this->properties = basic;
      break;
   }
   }
}

void Material::serialize_yaml(c4::yml::NodeRef& node) const
{
   auto mat_template_str = to_string(this->material_template);
   node["template"] = {mat_template_str.data(), mat_template_str.size()};

   auto properties_node = node["properties"];
   properties_node |= ryml::MAP;
   switch (this->material_template) {
   case MaterialTemplate::Basic:
      std::get<MTProperties_Basic>(this->properties).serialize_yaml(properties_node);
      break;
   case MaterialTemplate::NormalMap:
      std::get<MTProperties_NormalMap>(this->properties).serialize_yaml(properties_node);
      break;
   case MaterialTemplate::FullPBR:
      std::get<MTProperties_FullPBR>(this->properties).serialize_yaml(properties_node);
      break;
   }
}

}// namespace triglav::render_objects