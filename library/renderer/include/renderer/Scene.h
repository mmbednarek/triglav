#pragma once

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

struct Camera
{
   glm::vec3 position;
   glm::vec3 lookDirection;
};

class Scene
{
 public:
   Scene(Renderer &renderer, Context3D &context3D, ShadowMap &shadowMap);

   void update() const;
   void add_object(SceneObject object);
   void compile_scene();
   void render() const;
   void render_shadow_map() const;

   void set_camera(Camera camera);
   void set_shadow_x(float x);

 private:
   Renderer &m_renderer;
   Context3D &m_context3D;
   ShadowMap &m_shadowMap;

   Camera m_shadowMapCamera{
           {-40, -40, -40},
           {0.6, 0.6, 0.6}
   };
   Camera m_camera{};

   std::vector<SceneObject> m_objects;
   std::vector<InstancedModel> m_instancedObjects;
};

}// namespace renderer