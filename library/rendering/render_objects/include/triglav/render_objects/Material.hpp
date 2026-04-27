#pragma once

#include "triglav/Name.hpp"
#include "triglav/geometry/Geometry.hpp"

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

geometry::VertexComponentFlags material_template_to_vertex_component_flags(MaterialTemplate mat_template);

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

// Vertex configuration

struct MaterialVertexLayoutInfo
{
   u32 index;
   geometry::VertexComponentFlags components;
   VertexShaderName vertex_shader;
   VertexShaderName vertex_shader_shadow_map;
   Name passthrough_buffer;
};

using MaterialVertexLayoutID = u32;

constexpr std::array<MaterialVertexLayoutInfo, 4> VERTEX_LAYOUT_INFOS{
   MaterialVertexLayoutInfo{
      .index = 0,
      .components = geometry::VertexComponent::Core | geometry::VertexComponent::Texture,
      .vertex_shader = make_rc_name("shader/bindless_geometry/vertex_vl0.vshader"),
      .vertex_shader_shadow_map = make_rc_name("shader/bindless_geometry/shadow_map_static.vshader"),
      .passthrough_buffer = make_name_id("occlusion_culling.passthrough.vl0"),
   },
   MaterialVertexLayoutInfo{
      .index = 1,
      .components = geometry::VertexComponent::Core | geometry::VertexComponent::Texture | geometry::VertexComponent::NormalMap,
      .vertex_shader = make_rc_name("shader/bindless_geometry/vertex_vl1.vshader"),
      .vertex_shader_shadow_map = make_rc_name("shader/bindless_geometry/shadow_map_static.vshader"),
      .passthrough_buffer = make_name_id("occlusion_culling.passthrough.vl1"),
   },
   MaterialVertexLayoutInfo{
      .index = 2,
      .components = geometry::VertexComponent::Core | geometry::VertexComponent::Texture | geometry::VertexComponent::Skeleton,
      .vertex_shader = make_rc_name("shader/bindless_geometry/vertex_vl2.vshader"),
      .vertex_shader_shadow_map = make_rc_name("shader/bindless_geometry/shadow_map_bones.vshader"),
      .passthrough_buffer = make_name_id("occlusion_culling.passthrough.vl2"),
   },
   MaterialVertexLayoutInfo{
      .index = 3,
      .components = geometry::VertexComponent::Core | geometry::VertexComponent::Texture | geometry::VertexComponent::NormalMap |
                    geometry::VertexComponent::Skeleton,
      .vertex_shader = make_rc_name("shader/bindless_geometry/vertex_vl3.vshader"),
      .vertex_shader_shadow_map = make_rc_name("shader/bindless_geometry/shadow_map_bones.vshader"),
      .passthrough_buffer = make_name_id("occlusion_culling.passthrough.vl3"),
   },
};

struct MaterialGeometryRenderInfo
{
   u32 index;
   u32 materialPropIndex;
   MaterialVertexLayoutID vertex_layout_id;
   FragmentShaderName fragment_shader;
   Name draw_call_buffer;
};

constexpr std::array<MaterialGeometryRenderInfo, 6> GEOMETRY_RENDER_INFOS{
   MaterialGeometryRenderInfo{
      .index = 0,
      .materialPropIndex = 0,
      .vertex_layout_id = 0,
      .fragment_shader = make_rc_name("shader/bindless_geometry/render_mt0.fshader"),
      .draw_call_buffer = make_name_id("occlusion_culling.visible_objects.mt0"),
   },
   MaterialGeometryRenderInfo{
      .index = 1,
      .materialPropIndex = 1,
      .vertex_layout_id = 1,
      .fragment_shader = make_rc_name("shader/bindless_geometry/render_mt1.fshader"),
      .draw_call_buffer = make_name_id("occlusion_culling.visible_objects.mt1"),
   },
   MaterialGeometryRenderInfo{
      .index = 2,
      .materialPropIndex = 2,
      .vertex_layout_id = 1,
      .fragment_shader = make_rc_name("shader/bindless_geometry/render_mt2.fshader"),
      .draw_call_buffer = make_name_id("occlusion_culling.visible_objects.mt2"),
   },
   MaterialGeometryRenderInfo{
      .index = 3,
      .materialPropIndex = 0,
      .vertex_layout_id = 2,
      .fragment_shader = make_rc_name("shader/bindless_geometry/render_mt0.fshader"),
      .draw_call_buffer = make_name_id("occlusion_culling.visible_objects.mt3"),
   },
   MaterialGeometryRenderInfo{
      .index = 4,
      .materialPropIndex = 1,
      .vertex_layout_id = 3,
      .fragment_shader = make_rc_name("shader/bindless_geometry/render_mt1.fshader"),
      .draw_call_buffer = make_name_id("occlusion_culling.visible_objects.mt4"),
   },
   MaterialGeometryRenderInfo{
      .index = 5,
      .materialPropIndex = 2,
      .vertex_layout_id = 3,
      .fragment_shader = make_rc_name("shader/bindless_geometry/render_mt2.fshader"),
      .draw_call_buffer = make_name_id("occlusion_culling.visible_objects.mt5"),
   },
};

using MaterialGeometryRenderInfoID = u32;

}// namespace triglav::render_objects