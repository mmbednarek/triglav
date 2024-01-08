#pragma once

#include "Camera.h"
#include "DebugLinesRenderer.h"
#include "Model.h"
#include "Name.hpp"
#include "ShadowMap.h"

#include <glm/gtx/quaternion.hpp>
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
   Scene(Renderer &renderer, Context3D &context3D, ShadowMap &shadowMap, DebugLinesRenderer &debugLines,
         ResourceManager &resourceManager);

   void update();
   void add_object(SceneObject object);
   void compile_scene();
   void render() const;
   void render_shadow_map() const;
   void render_debug_lines() const;

   void set_camera(glm::vec3 position, glm::quat orientation);

   [[nodiscard]] const Camera &camera() const;
   [[nodiscard]] Camera &camera();

 private:
   Renderer &m_renderer;
   Context3D &m_context3D;
   ShadowMap &m_shadowMap;
   ResourceManager &m_resourceManager;
   DebugLinesRenderer &m_debugLinesRenderer;

   Camera m_shadowMapCamera{};
   Camera m_camera{};

   std::vector<SceneObject> m_objects;
   std::vector<InstancedModel> m_instancedObjects;
   std::vector<DebugLines> m_debugLines;
};

}// namespace renderer