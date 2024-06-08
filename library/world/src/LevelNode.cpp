#include "LevelNode.h"

namespace triglav::world {

void LevelNode::add_static_mesh(StaticMesh&& mesh)
{
   m_staticMeshes.emplace_back(mesh);
}

const std::vector<StaticMesh>& LevelNode::static_meshes()
{
   return m_staticMeshes;
}

}// namespace triglav::world