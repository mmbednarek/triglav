#include "LevelNode.hpp"

#include "triglav/ResourcePathMap.hpp"

#include <ryml.hpp>

namespace triglav::world {

namespace {

void serialize_vector3(ryml::NodeRef& node, const Vector3& vec3)
{
   std::string x_str = std::to_string(vec3.x);
   std::string y_str = std::to_string(vec3.y);
   std::string z_str = std::to_string(vec3.z);

   node["x"] << c4::substr{x_str.data(), x_str.size()};
   node["y"] << c4::substr{y_str.data(), y_str.size()};
   node["z"] << c4::substr{z_str.data(), z_str.size()};
}

void serialize_vector4(ryml::NodeRef& node, const Vector4& vec3)
{
   std::string x_str = std::to_string(vec3.x);
   std::string y_str = std::to_string(vec3.y);
   std::string z_str = std::to_string(vec3.z);
   std::string w_str = std::to_string(vec3.w);

   node["x"] << c4::substr{x_str.data(), x_str.size()};
   node["y"] << c4::substr{y_str.data(), y_str.size()};
   node["z"] << c4::substr{z_str.data(), z_str.size()};
   node["w"] << c4::substr{w_str.data(), w_str.size()};
}

void serialize_transform(ryml::NodeRef& node, const Transform3D& transform)
{
   auto translation_node = node["translation"];
   translation_node |= ryml::MAP;
   serialize_vector3(translation_node, transform.translation);

   auto rotation_node = node["rotation"];
   rotation_node |= ryml::MAP;
   glm::quat rotation{transform.rotation};
   serialize_vector4(rotation_node, {rotation.x, rotation.y, rotation.z, rotation.w});

   auto scale_node = node["scale"];
   scale_node |= ryml::MAP;
   serialize_vector3(scale_node, transform.scale);
}

}// namespace

void StaticMesh::serialize_yaml(ryml::NodeRef& node) const
{
   node["type"] = c4::csubstr{"static_mesh"};
   node["name"] = c4::csubstr{this->name.data(), this->name.size()};
   const auto mesh_str = ResourcePathMap::the().resolve(this->mesh_name);
   node["mesh"] << c4::csubstr{mesh_str.data(), mesh_str.size()};
   auto transform_node = node["transform"];
   transform_node |= ryml::MAP;
   serialize_transform(transform_node, this->transform);
}

LevelNode::LevelNode(const std::string_view name) :
    m_name(name)
{
}

void LevelNode::add_static_mesh(StaticMesh&& mesh)
{
   m_static_meshes.emplace_back(mesh);
}

const std::vector<StaticMesh>& LevelNode::static_meshes()
{
   return m_static_meshes;
}

void LevelNode::serialize_yaml(ryml::NodeRef& node) const
{
   node["name"] = c4::csubstr{this->m_name.data(), this->m_name.size()};
   auto items = node["items"];
   items |= ryml::SEQ;
   for (const auto& static_mesh : m_static_meshes) {
      auto child = items.append_child();
      child |= ryml::MAP;
      static_mesh.serialize_yaml(child);
   }
}

}// namespace triglav::world