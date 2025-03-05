#pragma once

#include "Camera.hpp"
#include "DebugLinesRenderer.hpp"
#include "OrthoCamera.hpp"

#include "triglav/Name.hpp"
#include "triglav/event/Delegate.hpp"
#include "triglav/render_objects/Mesh.hpp"

#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace triglav::renderer {

class ModelRenderer;
class Renderer;

struct SceneObject
{
   MeshName model;
   Transform3D transform;

   [[nodiscard]] Matrix4x4 model_matrix() const;
};

class Scene
{
 public:
   TG_EVENT(OnObjectAddedToScene, const SceneObject&)
   TG_EVENT(OnViewportChange, const graphics_api::Resolution&)
   TG_EVENT(OnAddedBoundingBox, const geometry::BoundingBox&)
   TG_EVENT(OnShadowMapChanged, u32, const OrthoCamera&)
   TG_EVENT(OnViewUpdated, const Camera&)

   explicit Scene(resource::ResourceManager& resourceManager);

   void update(graphics_api::Resolution& resolution);
   void add_object(SceneObject object);
   void load_level(LevelName name);
   void set_camera(glm::vec3 position, glm::quat orientation);
   void update_shadow_maps();
   void send_view_changed();

   [[nodiscard]] const Camera& camera() const;
   [[nodiscard]] Camera& camera();
   [[nodiscard]] const OrthoCamera& shadow_map_camera(u32 index) const;
   [[nodiscard]] u32 directional_shadow_map_count() const;

   [[nodiscard]] float yaw() const;
   [[nodiscard]] float pitch() const;

   void update_orientation(float delta_yaw, float delta_pitch);
   void add_bounding_box(const geometry::BoundingBox& box);

 private:
   resource::ResourceManager& m_resourceManager;
   float m_yaw{};
   float m_pitch{};
   Camera m_camera{};
   glm::quat m_directionalLightOrientation{glm::vec3{geometry::g_pi * 0.1f, 0, geometry::g_pi * 1.5f}};
   std::array<OrthoCamera, 3> m_directionalShadowMapCameras{};
   std::vector<SceneObject> m_objects{};
};

}// namespace triglav::renderer