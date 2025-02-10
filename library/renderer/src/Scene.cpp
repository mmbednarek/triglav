#include "Scene.hpp"

#include "Renderer.hpp"

#include "triglav/world/Level.hpp"

#include <cmath>
#include <glm/gtc/quaternion.hpp>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

using triglav::ResourceType;

namespace triglav::renderer {

glm::mat4 SceneObject::model_matrix() const
{
   return glm::scale(glm::translate(glm::mat4(1), this->position), this->scale) * glm::mat4_cast(this->rotation);
}

Scene::Scene(resource::ResourceManager& resourceManager) :
    m_resourceManager(resourceManager)
{
}

void Scene::update(graphics_api::Resolution& resolution)
{
   const auto [width, height] = resolution;
   m_camera.set_viewport_size(static_cast<float>(width), static_cast<float>(height));

   event_OnViewportChange.publish(resolution);
}

void Scene::add_object(SceneObject object)
{
   auto& emplacedObj = m_objects.emplace_back(object);
   event_OnObjectAddedToScene.publish(emplacedObj);
}

void Scene::load_level(const LevelName name)
{
   auto& level = m_resourceManager.get<ResourceType::Level>(name);
   auto& root = level.root();

   for (const auto& mesh : root.static_meshes()) {
      this->add_object(SceneObject{
         .model = mesh.meshName,
         .position = mesh.transform.position,
         .rotation = glm::quat(mesh.transform.rotation),
         .scale = mesh.transform.scale,
      });
   }
}

void Scene::set_camera(const glm::vec3 position, const glm::quat orientation)
{
   m_camera.set_position(position);
   m_camera.set_orientation(orientation);
   event_OnViewUpdated.publish(m_camera);
}

const Camera& Scene::camera() const
{
   return m_camera;
}

Camera& Scene::camera()
{
   return m_camera;
}

const OrthoCamera& Scene::shadow_map_camera(const u32 index) const
{
   return m_directionalShadowMapCameras[index];
}

u32 Scene::directional_shadow_map_count() const
{
   return m_directionalShadowMapCameras.size();
}

float Scene::yaw() const
{
   return m_yaw;
}

float Scene::pitch() const
{
   return m_pitch;
}

void Scene::update_orientation(const float delta_yaw, const float delta_pitch)
{
   m_yaw += delta_yaw;
   while (m_yaw < 0) {
      m_yaw += 2 * M_PI;
   }
   while (m_yaw >= 2 * M_PI) {
      m_yaw -= 2 * M_PI;
   }

   m_pitch += delta_pitch;
   m_pitch = std::clamp(m_pitch, -static_cast<float>(M_PI) / 2.0f + 0.01f, static_cast<float>(M_PI) / 2.0f - 0.01f);

   this->camera().set_orientation(glm::quat{glm::vec3{m_pitch, 0.0f, m_yaw}});
}

void Scene::add_bounding_box(const geometry::BoundingBox& box)
{
   event_OnAddedBoundingBox.publish(box);
}

void Scene::update_shadow_maps()
{
   auto smProps1 = this->camera().calculate_shadow_map(m_directionalLightOrientation, 32.0f, 120.0f);
   m_directionalShadowMapCameras[0] = OrthoCamera::from_properties(smProps1);
   event_OnShadowMapChanged.publish(0, m_directionalShadowMapCameras[0]);

   auto smProps2 = this->camera().calculate_shadow_map(m_directionalLightOrientation, 72.0f, 192.0f);
   m_directionalShadowMapCameras[1] = OrthoCamera::from_properties(smProps2);
   event_OnShadowMapChanged.publish(1, m_directionalShadowMapCameras[1]);

   auto smProps3 = this->camera().calculate_shadow_map(m_directionalLightOrientation, 180.0f, 256.0f);
   m_directionalShadowMapCameras[2] = OrthoCamera::from_properties(smProps3);
   event_OnShadowMapChanged.publish(2, m_directionalShadowMapCameras[2]);
}

void Scene::send_view_changed()
{
   event_OnViewUpdated.publish(m_camera);
}

}// namespace triglav::renderer