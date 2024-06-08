#pragma once

#include "Camera.h"
#include "DebugLinesRenderer.h"
#include "OrthoCamera.h"

#include "triglav/Delegate.hpp"
#include "triglav/Name.hpp"
#include "triglav/render_core/Model.hpp"

#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace triglav::renderer {

class ModelRenderer;
class Renderer;

struct SceneObject
{
   triglav::ModelName model;
   glm::vec3 position;
   glm::quat rotation;
   glm::vec3 scale;

   [[nodiscard]] glm::mat4 model_matrix() const;
};

class Scene
{
 public:
   using OnObjectAddedToSceneDel = Delegate<const SceneObject&>;
   using OnViewportChangeDel = Delegate<const graphics_api::Resolution&>;

   OnObjectAddedToSceneDel OnObjectAddedToScene;
   OnViewportChangeDel OnViewportChange;

   explicit Scene(resource::ResourceManager& resourceManager);

   void update(graphics_api::Resolution& resolution);
   void add_object(SceneObject object);
   void load_level(LevelName name);
   void set_camera(glm::vec3 position, glm::quat orientation);

   [[nodiscard]] const Camera& camera() const;
   [[nodiscard]] Camera& camera();
   [[nodiscard]] const OrthoCamera& shadow_map_camera() const;

   [[nodiscard]] float yaw() const;
   [[nodiscard]] float pitch() const;

   void update_orientation(float delta_yaw, float delta_pitch);

 private:
   resource::ResourceManager& m_resourceManager;
   float m_yaw{};
   float m_pitch{};

   OrthoCamera m_shadowMapCamera{};
   Camera m_camera{};

   std::vector<SceneObject> m_objects{};
};

}// namespace triglav::renderer