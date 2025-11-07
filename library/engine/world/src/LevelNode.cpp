#include "LevelNode.hpp"

#include <ryml.hpp>

namespace triglav::world {

namespace {

void serialize_vector3(ryml::NodeRef& node, const Vector3& vec3)
{
   std::string xStr = std::to_string(vec3.x);
   std::string yStr = std::to_string(vec3.y);
   std::string zStr = std::to_string(vec3.z);

   node["x"] << c4::substr{xStr.data(), xStr.size()};
   node["y"] << c4::substr{yStr.data(), yStr.size()};
   node["z"] << c4::substr{zStr.data(), zStr.size()};
}

void serialize_vector4(ryml::NodeRef& node, const Vector4& vec3)
{
   std::string xStr = std::to_string(vec3.x);
   std::string yStr = std::to_string(vec3.y);
   std::string zStr = std::to_string(vec3.z);
   std::string wStr = std::to_string(vec3.w);

   node["x"] << c4::substr{xStr.data(), xStr.size()};
   node["y"] << c4::substr{yStr.data(), yStr.size()};
   node["z"] << c4::substr{zStr.data(), zStr.size()};
   node["w"] << c4::substr{wStr.data(), wStr.size()};
}

void serialize_transform(ryml::NodeRef& node, const Transform3D& transform)
{
   auto translationNode = node["translation"];
   translationNode |= ryml::MAP;
   serialize_vector3(translationNode, transform.translation);

   auto rotationNode = node["rotation"];
   rotationNode |= ryml::MAP;
   glm::quat rotation{transform.rotation};
   serialize_vector4(rotationNode, {rotation.x, rotation.y, rotation.z, rotation.w});

   auto scaleNode = node["scale"];
   scaleNode |= ryml::MAP;
   serialize_vector3(scaleNode, transform.scale);
}

}// namespace

void StaticMesh::serialize_yaml(ryml::NodeRef& node) const
{
   node["type"] = c4::csubstr{"static_mesh"};
   node["name"] = c4::csubstr{this->name.data(), this->name.size()};
   const auto meshStr = std::to_string(this->meshName.name());
   node["mesh"] << c4::csubstr{meshStr.data(), meshStr.size()};
   auto transformNode = node["transform"];
   transformNode |= ryml::MAP;
   serialize_transform(transformNode, this->transform);
}

LevelNode::LevelNode(const std::string_view name) :
    m_name(name)
{
}

void LevelNode::add_static_mesh(StaticMesh&& mesh)
{
   m_staticMeshes.emplace_back(mesh);
}

const std::vector<StaticMesh>& LevelNode::static_meshes()
{
   return m_staticMeshes;
}

void LevelNode::serialize_yaml(ryml::NodeRef& node) const
{
   node["name"] = c4::csubstr{this->m_name.data(), this->m_name.size()};
   auto items = node["items"];
   items |= ryml::SEQ;
   for (const auto& staticMesh : m_staticMeshes) {
      auto child = items.append_child();
      child |= ryml::MAP;
      staticMesh.serialize_yaml(child);
   }
}

}// namespace triglav::world