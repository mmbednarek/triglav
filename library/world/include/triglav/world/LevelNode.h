#pragma once

#include "triglav/Name.hpp"

#include <glm/vec3.hpp>
#include <memory>
#include <vector>

namespace triglav::world {

struct Transformation {
  glm::vec3 position{};
  glm::vec3 rotation{};
  glm::vec3 scale{};
};

struct StaticMesh {
  ModelName meshName{0};
  Transformation transform{};
};

class LevelNode {
public:
  void add_static_mesh(StaticMesh&& mesh);

   const std::vector<StaticMesh>& static_meshes();


private:
  std::vector<StaticMesh> m_staticMeshes;
  std::vector<NameID> m_children;
};

}