#include "Scene.hpp"

#include "Renderer.hpp"

#include "triglav/world/Level.hpp"

#include <cmath>
#include <glm/gtc/quaternion.hpp>

using triglav::ResourceType;

namespace triglav::renderer {

using namespace name_literals;

Matrix4x4 SceneObject::model_matrix() const
{
   return this->transform.to_matrix();
}

Scene::Scene(resource::ResourceManager& resource_manager) :
    m_resource_manager(resource_manager)
{
}

void Scene::update(const graphics_api::Resolution& resolution)
{
   const auto [width, height] = resolution;
   m_camera.set_viewport_size(static_cast<float>(width), static_cast<float>(height));

   event_OnViewportChange.publish(resolution);
   this->send_view_changed();
}

ObjectID Scene::add_object(SceneObject object)
{
   const auto object_id = static_cast<ObjectID>(m_objects.size());

   auto& emplaced_obj = m_objects.emplace_back(std::move(object));
   event_OnObjectAddedToScene.publish(object_id, emplaced_obj);
   this->update_bvh();

   return object_id;
}

void Scene::set_transform(const ObjectID object_id, const Transform3D& transform)
{
   m_objects[object_id].transform = transform;
   this->update_bvh();

   event_OnObjectChangedTransform.publish(object_id, transform);
}

void Scene::load_level(const LevelName name)
{
   auto& level = m_resource_manager.get<ResourceType::Level>(name);
   auto& root = level.root();

   for (const auto& mesh : root.static_meshes()) {
      this->add_object(SceneObject{
         .model = mesh.mesh_name,
         .name = mesh.name.c_str(),
         .transform = mesh.transform,
      });
   }
}

world::Level Scene::to_level() const
{
   world::LevelNode root_node("root");
   for (const auto& object : m_objects) {
      root_node.add_static_mesh(world::StaticMesh{
         .mesh_name = object.model,
         .name = object.name.to_std(),
         .transform = object.transform,
      });
   }

   world::Level result;
   result.add_node("root"_name, std::move(root_node));
   return result;
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
   return m_directional_shadow_map_cameras[index];
}

u32 Scene::directional_shadow_map_count() const
{
   return static_cast<u32>(m_directional_shadow_map_cameras.size());
}

const SceneObject& Scene::object(const ObjectID id) const
{
   return m_objects[id];
}

float Scene::yaw() const
{
   return m_yaw;
}

float Scene::pitch() const
{
   return m_pitch;
}

void Scene::update_bvh()
{
   std::vector<SceneObjectRef> objects{m_objects.size()};
   std::ranges::transform(m_objects, objects.begin(), [this, index = 0u](SceneObject& object) mutable {
      const auto& mesh = m_resource_manager.get(object.model);
      return SceneObjectRef{
         .object = &object,
         .bbox = mesh.bounding_box.transform(object.transform.to_matrix()),
         .id = index++,
      };
   });
   m_tree.build(objects);
}

void Scene::update_orientation(const float delta_yaw, const float delta_pitch)
{
   m_yaw += delta_yaw;
   while (m_yaw < 0) {
      m_yaw += static_cast<float>(2 * g_pi);
   }
   while (m_yaw >= static_cast<float>(2 * g_pi)) {
      m_yaw -= static_cast<float>(2 * g_pi);
   }

   m_pitch += delta_pitch;
   m_pitch = std::clamp(m_pitch, -static_cast<float>(g_pi) / 2.0f + 0.01f, static_cast<float>(g_pi) / 2.0f - 0.01f);

   this->camera().set_orientation(glm::quat{glm::vec3{m_pitch, 0.0f, m_yaw}});
   this->send_view_changed();
}

void Scene::add_bounding_box(const geometry::BoundingBox& box) const
{
   event_OnAddedBoundingBox.publish(box);
}

const geometry::BVHTree<SceneObjectRef>& Scene::bvh() const
{
   return m_tree;
}

RayHit Scene::trace_ray(const geometry::Ray& ray) const
{
   const auto hit = this->bvh().traverse(ray);
   if (hit.payload == nullptr)
      return {INFINITY, ~0u, nullptr};
   return {hit.distance, hit.payload->id, hit.payload->object};
}

void Scene::update_shadow_maps()
{
   auto sm_props1 = this->camera().calculate_shadow_map(m_directional_light_orientation, 32.0f, 120.0f);
   m_directional_shadow_map_cameras[0] = OrthoCamera::from_properties(sm_props1);
   event_OnShadowMapChanged.publish(0, m_directional_shadow_map_cameras[0]);

   auto sm_props2 = this->camera().calculate_shadow_map(m_directional_light_orientation, 72.0f, 192.0f);
   m_directional_shadow_map_cameras[1] = OrthoCamera::from_properties(sm_props2);
   event_OnShadowMapChanged.publish(1, m_directional_shadow_map_cameras[1]);

   auto sm_props3 = this->camera().calculate_shadow_map(m_directional_light_orientation, 180.0f, 256.0f);
   m_directional_shadow_map_cameras[2] = OrthoCamera::from_properties(sm_props3);
   event_OnShadowMapChanged.publish(2, m_directional_shadow_map_cameras[2]);
}

void Scene::send_view_changed()
{
   event_OnViewUpdated.publish(m_camera);
}

}// namespace triglav::renderer