#pragma once

#include "Camera.h"
#include "DebugLinesRenderer.h"
#include "ModelRenderer.h"
#include "OrthoCamera.h"

#include "triglav/Name.hpp"
#include "triglav/render_core/Model.hpp"

#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace triglav::renderer {

class ModelRenderer;
class Renderer;

struct SceneObject
{
   triglav::Name model;
   glm::vec3 position;
   glm::quat rotation;
   glm::vec3 scale;
};

class Scene
{
 public:
   Scene(ModelRenderer &context3D, ShadowMapRenderer &shadowMap, DebugLinesRenderer &debugLines,
         triglav::resource::ResourceManager &resourceManager);

   void update(graphics_api::Resolution &resolution);
   void add_object(SceneObject object);
   void compile_scene();
   void render(graphics_api::CommandList &cmdList) const;
   void render_shadow_map(graphics_api::CommandList &cmdList) const;
   void render_debug_lines(graphics_api::CommandList &cmdList) const;

   void load_level(LevelName name);

   void set_camera(glm::vec3 position, glm::quat orientation);

   [[nodiscard]] const Camera &camera() const;
   [[nodiscard]] Camera &camera();
   [[nodiscard]] const OrthoCamera &shadow_map_camera() const;

   [[nodiscard]] float yaw() const;
   [[nodiscard]] float pitch() const;

   [[nodiscard]] void update_orientation(float delta_yaw, float delta_pitch);

 private:
   ModelRenderer &m_context3D;
   ShadowMapRenderer &m_shadowMap;
   resource::ResourceManager &m_resourceManager;
   DebugLinesRenderer &m_debugLinesRenderer;
   float m_yaw{};
   float m_pitch{};

   OrthoCamera m_shadowMapCamera{};
   Camera m_camera{};

   std::vector<SceneObject> m_objects;
   std::vector<triglav::render_core::InstancedModel> m_instancedObjects;
   std::vector<DebugLines> m_debugLines;
};

}// namespace triglav::renderer