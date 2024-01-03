#pragma once

#include "Camera.h"
#include "Model.h"
#include "Name.hpp"
#include "ShadowMap.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/vec3.hpp>
#include <vector>

namespace renderer {

class Context3D;
class Renderer;

struct SceneObject
{
   Name model;
   glm::vec3 position;
   glm::quat rotation;
   glm::vec3 scale;
};

class Scene
{
 public:
   Scene(Renderer &renderer, Context3D &context3D, ShadowMap &shadowMap);

   void update();
   void add_object(SceneObject object);
   void compile_scene();
   void render() const;
   void render_shadow_map() const;

   void set_camera(glm::vec3 position, glm::quat orientation);

   [[nodiscard]] const Camera &camera() const;

 private:
   Renderer &m_renderer;
   Context3D &m_context3D;
   ShadowMap &m_shadowMap;

   Camera m_shadowMapCamera{};
   Camera m_camera{};

   std::vector<SceneObject> m_objects;
   std::vector<InstancedModel> m_instancedObjects;
};

}// namespace renderer