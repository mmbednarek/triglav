#pragma once

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"

#include <string>
#include <vector>

namespace c4::yml {
class NodeRef;
}

namespace triglav::world {

struct StaticMesh
{
   MeshName meshName{0};
   std::string name;
   Transform3D transform{};

   void serialize_yaml(c4::yml::NodeRef& node) const;
};

class LevelNode
{
 public:
   explicit LevelNode(std::string_view name);

   void add_static_mesh(StaticMesh&& mesh);

   const std::vector<StaticMesh>& static_meshes();

   void serialize_yaml(c4::yml::NodeRef& node) const;

 private:
   std::string m_name;
   std::vector<StaticMesh> m_staticMeshes;
   std::vector<Name> m_children;
};

}// namespace triglav::world