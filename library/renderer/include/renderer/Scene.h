#pragma once

#include "Name.hpp"
#include "Model.h"

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
   Scene(Renderer& renderer, Context3D& context3D);

   void update() const;
   void add_object(SceneObject object);
   void compile_scene();
   void render() const;

   void set_camera(Camera camera);

private:
   Renderer& m_renderer;
   Context3D& m_context3D;

   Camera m_camera{};

   std::vector<SceneObject> m_objects;
   std::vector<InstancedModel> m_instancedObjects;
};

}